// StationeryTree.cpp : implementation file
//

#include "stdafx.h"
#include "eudora.h"
#include "guiutils.h"
#include "ClipboardMgr.h"
#include "QCCommandActions.h"
#include "QCStationeryCommand.h"
#include "QCStationeryDirector.h"
#include "rs.h"
#include "StationeryTree.h"
#include "QCOleDropSource.h"

extern QCStationeryDirector	g_theStationeryDirector;

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

extern CString EudoraDir;

/////////////////////////////////////////////////////////////////////////////
// CStationeryTree

CStationeryTree::CStationeryTree()
{
	g_theStationeryDirector.Register( this );
}

CStationeryTree::~CStationeryTree()
{
	g_theStationeryDirector.UnRegister( this );
}

BEGIN_MESSAGE_MAP(CStationeryTree, QCTreeCtrl)
	//{{AFX_MSG_MAP(CStationeryTree)
	ON_NOTIFY_REFLECT(TVN_BEGINLABELEDIT, OnBeginLabelEdit)
	ON_NOTIFY_REFLECT(TVN_ENDLABELEDIT, OnEndLabelEdit)
	ON_WM_CHAR()
	ON_WM_KEYUP()
	ON_NOTIFY_REFLECT(TVN_BEGINDRAG, OnBeginDrag)
	ON_WM_CONTEXTMENU()
	ON_WM_LBUTTONDBLCLK()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(CStationeryTree, QCTreeCtrl)
	//{{AFX_DISPATCH_MAP(CStationeryTree)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()

// Note: we add support for IID_IStationeryTree to support typesafe binding
//  from VBA.  This IID must match the GUID that is attached to the 
//  dispinterface in the .ODL file.

// {B86B71B0-A488-11D0-97B0-00805FD2F268}
static const IID IID_IStationeryTree =
{ 0xb86b71b0, 0xa488, 0x11d0, { 0x97, 0xb0, 0x0, 0x80, 0x5f, 0xd2, 0xf2, 0x68 } };

BEGIN_INTERFACE_MAP(CStationeryTree, QCTreeCtrl)
	INTERFACE_PART(CStationeryTree, IID_IStationeryTree, Dispatch)
END_INTERFACE_MAP()

void CStationeryTree::OnFinalRelease()
{
	// When the last reference for an automation object is released
	// OnFinalRelease is called.  The base class will automatically
	// deletes the object.  Add additional cleanup required for your
	// object before calling the base class.

	QCTreeCtrl::OnFinalRelease();
}


BOOL CStationeryTree::Init()
{
	QCStationeryCommand*	pCommand;
	POSITION				pos;

	if (! QCTreeCtrl::Init())
		return FALSE;

	//
	// Add the bitmap images for tree control.
	//
	if (! m_ImageList.Load())
	{
		return FALSE;		// mangled resources?
	}

	SetImageList(&m_ImageList, TVSIL_NORMAL);
	
	//
	// Set "thin" font for old Win 3.x GUI.
	//
	if (! IsVersion4())
	{
		SetFont(CFont::FromHandle(HFONT(::GetStockObject(ANSI_VAR_FONT))));
	}

	pos = g_theStationeryDirector.GetFirstStationeryPosition();

	while( pos != NULL )
	{
		pCommand = g_theStationeryDirector.GetNext( pos );
		AddItem( pCommand->GetName() );
	}
	
	
	SortChildren( NULL );

	return TRUE;
}



DROPEFFECT CStationeryTree::OnDragEnter(
COleDataObject*	pDataObject,
DWORD, 
CPoint)
{
	if( g_theClipboardMgr.IsInstanceDataAvailable( pDataObject, QCClipboardMgr::CF_TRANSFER ) )
	{
		StartAutoScrollTimer();
	}

	return DROPEFFECT_NONE;
}


void CStationeryTree::OnDragLeave()
{
	KillAutoScrollTimer();
}


DROPEFFECT CStationeryTree::OnDragOver(
COleDataObject*	pDataObject,
DWORD, 
CPoint			thePoint)
{
	if( g_theClipboardMgr.IsInstanceDataAvailable( pDataObject, QCClipboardMgr::CF_TRANSFER ) )
	{
		UINT flags = TVHT_ONITEM | TVHT_ONITEMBUTTON | TVHT_ONITEMINDENT | TVHT_ONITEMRIGHT;
		HTREEITEM h_target = HitTest( thePoint, &flags);
		if (h_target != NULL)
		{
			SelectDropTarget(h_target);
			return DROPEFFECT_MOVE;
		}
#ifdef OK1
		QCFTRANSFER qcfTransfer;
		if (! g_theClipboardMgr.GetGlobalData(pDataObject, qcfTransfer))
		{
			ASSERT(0);
			return DROPEFFECT_NONE;
		}

		//
		// Okay, clipboard has usable drop data from our instance,
		// so let's do a hit test to see if we're over a valid 
		// mailbox target.
		//
		UINT flags = TVHT_ONITEM | TVHT_ONITEMBUTTON | TVHT_ONITEMINDENT | TVHT_ONITEMRIGHT;
		HTREEITEM h_target = HitTest(point, &flags);
		if (h_target != NULL)
		{
			CMboxTreeItemData* p_itemdata = (CMboxTreeItemData *) GetItemData(h_target);
			ASSERT(p_itemdata != NULL);
			switch (p_itemdata->m_itemType)
			{
			case ITEM_ROOT:
			case ITEM_FOLDER:
				{
					//
					// Hit folder item, so select it using bogus "link" cursor
					// to give user visual feedback regarding "auto-open"
					// capability.
					//
					SelectDropTarget(h_target);
					return DROPEFFECT_LINK;
				}
				break;
			case ITEM_IN_MBOX:
			case ITEM_OUT_MBOX:
			case ITEM_TRASH_MBOX:
			case ITEM_USER_MBOX:
				SelectDropTarget(h_target);
				if ((dwKeyState & MK_CONTROL) || ShiftDown())
					return DROPEFFECT_COPY;
				else
					return DROPEFFECT_MOVE;
				break;
			default:
				ASSERT(0);
				break;
			}
		}
#endif
	}

	SelectDropTarget(NULL);
	return DROPEFFECT_NONE;
}


/////////////////////////////////////////////////////////////////////////////
// CStationeryTree message handlers

HTREEITEM CStationeryTree::AddItem(LPCSTR szName)
{
	return InsertItem( szName, QCMailboxImageList::IMAGE_STATIONERY, QCMailboxImageList::IMAGE_STATIONERY );
}

void CStationeryTree::OnBeginLabelEdit(NMHDR* pNMHDR, LRESULT* pResult) 
{
	TV_DISPINFO* pTVDispInfo = (TV_DISPINFO*)pNMHDR;
	// TODO: Add your control notification handler code here
	
	*pResult = 0;
}

void CStationeryTree::OnEndLabelEdit(NMHDR* pNMHDR, LRESULT* pResult) 
{
	TV_DISPINFO*			pTVDispInfo;
	CString					szOldName;
	CEdit*					pEdit;
	CString					szName;
	QCStationeryCommand*	pCommand;

	pTVDispInfo = (TV_DISPINFO*)pNMHDR;
	*pResult = 0;

	if( ( pTVDispInfo->item.mask & TVIF_TEXT ) == 0 )
	{
		// the user pressed escape
		return;
	}

	szName = pTVDispInfo->item.pszText;
	szName.TrimLeft();
	szName.TrimRight();
	
	if( szName == "" )
	{
		// don't let the user set this to a blank name 
		// go back to editing

		MessageBeep( MB_OK );
		SendMessage( TVM_EDITLABEL, 0, LPARAM( pTVDispInfo->item.hItem ) );
		
		pEdit = GetEditControl();
		
		if( pEdit != NULL )
		{
			pEdit->SetWindowText( szName );
			pEdit->SetSel( 0, -1 );
		}
		
		return;
	}

	szOldName = GetItemText( pTVDispInfo->item.hItem );
	
	pCommand = g_theStationeryDirector.Find( szOldName );

	if( pCommand != NULL )
	{
		pCommand->Execute( CA_RENAME, ( void* ) (LPCSTR) szName );
	}

	if( szOldName.CompareNoCase( szName ) != 0 )
	{
		pCommand = g_theStationeryDirector.Find( szOldName );

		if( pCommand )
		{
			// the old name is still around -- an error occurred
			// keep editing after failure
			
			SendMessage( TVM_EDITLABEL, 0, LPARAM( pTVDispInfo->item.hItem ) );
			
			pEdit = GetEditControl();
			
			if( pEdit != NULL )
			{
				pEdit->SetWindowText( szName );
				pEdit->SetSel( 0, -1 );
			}
		}
	}			
}

void CStationeryTree::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	if( nChar == VK_RETURN ) 
	{
		if (GetEditControl())
			SendMessage( TVM_ENDEDITLABELNOW, FALSE, 0 );
		else
		{
			HTREEITEM h_selitem = GetSelectedItem();

			if (h_selitem)
			{
//				CWnd* pParentWnd = GetParent();
				CWnd* pParentWnd = AfxGetMainWnd();
				ASSERT(pParentWnd != NULL);
				pParentWnd->PostMessage(WM_COMMAND, IDM_NEW_MESSAGE_WITH_STATIONERY);
			}
		}
	}
	else
	if( nChar == VK_ESCAPE )
	{
		SendMessage( TVM_ENDEDITLABELNOW, TRUE, 0 );	
	}
	else
	{
		QCTreeCtrl::OnChar(nChar, nRepCnt, nFlags);
	}
}


////////////////////////////////////////////////////////////////////////
// OnKeyUp [protected]
//
////////////////////////////////////////////////////////////////////////
void CStationeryTree::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	switch (nChar)
	{
	case VK_DELETE:
		{
			// The Del key is a shortcut for the Delete command.
			CWnd* pParentWnd = GetParent();
			ASSERT(pParentWnd != NULL);
			pParentWnd->PostMessage(WM_COMMAND, IDM_DELETE_STATIONERY, LPARAM(GetSafeHwnd()));
		}
		break;
	case VK_F2:
		{
			//
			// Shortcut for the Rename command.
			//
			HTREEITEM hSelectedItem = GetSelectedItem();
			if (hSelectedItem != NULL)
				PostMessage(TVM_EDITLABEL, 0, LPARAM(hSelectedItem)); 
			QCTreeCtrl::OnKeyUp(nChar, nRepCnt, nFlags);
		}
		break;
	default:
		QCTreeCtrl::OnKeyUp(nChar, nRepCnt, nFlags);
		break;
	}
}



void CStationeryTree::OnBeginDrag(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW*				pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	QCStationeryCommand*		pCommand;
	HTREEITEM					hItem;
	CString						szName;

	hItem = pNMTreeView->itemNew.hItem;
	szName = GetItemText( hItem );

	// find the command
	pCommand = g_theStationeryDirector.Find( szName );

	if( pCommand == NULL )
	{
		ASSERT( 0 );
		return;
	}

	// allocate space
	QCFSTATIONERY qcfStationery;
	ASSERT(pCommand->GetPathname().GetLength() < sizeof(qcfStationery.m_szPathName));
	strcpy(qcfStationery.m_szPathName, pCommand->GetPathname() );
	g_theClipboardMgr.CacheGlobalData(m_OleDataSource, qcfStationery);
	
	//
	// Do the drag and drop
	//
	QCOleDropSource oleDropSource;
	DROPEFFECT theDropEffect = m_OleDataSource.DoDragDrop( DROPEFFECT_COPY | DROPEFFECT_MOVE | DROPEFFECT_LINK, NULL, &oleDropSource);
  
	// Clear the Data Source's cache
	m_OleDataSource.Empty();
	
	*pResult = 0;
}


HTREEITEM	CStationeryTree::FindItem(
HTREEITEM	hItem,
LPCSTR		szName )
{
	HTREEITEM	hChild;
	CString		szItemName;

	if( hItem == NULL )
	{
		return NULL;
	}
	
	szItemName = GetItemText( hItem );

	if( szItemName.CompareNoCase( szName ) == 0 )
	{
		return hItem;
	}

	if( ItemHasChildren( hItem ) )
	{
		// do a depth first search
		hChild = GetChildItem( hItem );

		if( hChild != NULL )
		{
			hChild = FindItem( hChild, szName );
			if( hChild != NULL )
			{
				// it's a child of this
				return hChild;
			}
		}
	}
	
	// check sibling items 
	return FindItem( GetNextSiblingItem( hItem ), szName );
}


void CStationeryTree::Notify( 
QCCommandObject*	pObject,
COMMAND_ACTION_TYPE	theAction,
void*				pData )
{
	HTREEITEM	hItem;

	if( ( pObject->IsKindOf( RUNTIME_CLASS( QCStationeryCommand ) ) == FALSE ) ||
		( ( theAction != CA_RENAME ) && ( theAction != CA_NEW ) && ( theAction != CA_DELETE ) ))
	{
		return;
	}

	if( ( theAction == CA_RENAME ) || ( theAction == CA_DELETE ) )
	{
		hItem = FindItem( GetRootItem(), ( ( QCStationeryCommand* ) pObject )->GetName() );
		
		if( hItem )
		{
			if( theAction == CA_DELETE ) 
			{
				DeleteItem( hItem );
			}
			else
			{
				SetItemText( hItem, ( LPCSTR ) pData );
				SortChildren( NULL );
			}
		}
	}

	if( theAction == CA_NEW ) 
	{
		AddItem( ( LPCSTR ) pData );
		SortChildren( NULL );
	}
}

void CStationeryTree::OnContextMenu(CWnd* pWnd, CPoint point) 
{
	HTREEITEM	hGetSelectedItem;
	HTREEITEM	hHitTest;
	UINT		uFlags;
	CPoint		thePoint( point );

	hGetSelectedItem = GetSelectedItem();
	ScreenToClient( &thePoint );
	hHitTest = HitTest( thePoint, &uFlags );

	if( ( hHitTest != hGetSelectedItem ) && ( hHitTest != NULL ) )
	{
		if( uFlags & TVHT_ONITEM )
		{
			SelectItem( hHitTest );	
		}
	}

	CTreeCtrl::OnContextMenu( pWnd, point );
}


////////////////////////////////////////////////////////////////////////
// OnLButtonDblClk [protected]
//
// Handle mouse double-click notification message by running the
// New Message With command on the selected stationery item, if any.
////////////////////////////////////////////////////////////////////////
void CStationeryTree::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	QCTreeCtrl::OnLButtonDblClk(nFlags, point);

	HTREEITEM h_selitem = GetSelectedItem();

	if (h_selitem)
	{
//		CWnd* pParentWnd = GetParent();
		CWnd* pParentWnd = AfxGetMainWnd();
		ASSERT(pParentWnd != NULL);
		pParentWnd->PostMessage(WM_COMMAND, IDM_NEW_MESSAGE_WITH_STATIONERY);
	}
}
