/////////////////////////////////////////////////////////////////////////////
//// LoadSys.c 代码实现文件
/*-----------------------------------------------------------
   LoadSys.c -- Load drive
       (c) Snow-dream E-mail:linbin_12345@163.com, 2008-08-16
  ----------------------------------------------------------*/
#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <commdlg.h>
#include <Shellapi.h>
#include "resource.h"

static TCHAR errmsg[MAX_PATH];
static void ReportError(HWND hwnd, TCHAR *upname) {
	DWORD dwLastError = GetLastError();
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dwLastError,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&upname,
		0,
		NULL);
	//TCharToOem(upname, upname);
	_stprintf_s(errmsg, sizeof(errmsg), _T("%d(0x%x):%s\n"), dwLastError, dwLastError, upname);
	SetWindowText(hwnd, errmsg);
	LocalFree(upname);
}

BOOL CALLBACK DlgProc (HWND, UINT, WPARAM, LPARAM) ;
BOOL operaType(TCHAR *szFullPath, TCHAR *szName, int iType) ; //操作类型(安装、运行、停止、移除)
void PopFileInitialize (HWND, OPENFILENAME *) ; //初始化打开文件对话框
BOOL PopFileOpenDlg (HWND, OPENFILENAME *, PTSTR, PTSTR) ; //弹出打开文件对话框

TCHAR szAppName [] = TEXT ("LoadSys") ;
HWND hwndStatus = NULL ; //状态标识的窗口句柄

int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    PSTR szCmdLine, int iCmdShow)
{
        if (-1 == DialogBox (hInstance, szAppName, NULL, (INT_PTR (CALLBACK *)(HWND, UINT, WPARAM, LPARAM))DlgProc))
        {
                MessageBox (NULL, TEXT ("This program requires Windows NT!"),
                        szAppName, MB_ICONERROR) ;
        }
        return 0 ;
}

BOOL CALLBACK DlgProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
        static OPENFILENAME ofn ;
        static HWND hwndDrvPath = NULL ;
        static TCHAR szFullPath[MAX_PATH]={0}, szTitle[MAX_PATH]={0} ;
        HDROP hDrop = NULL ;
        int iOperaType = 0 ;
        TCHAR *pStr = NULL ;

        switch (message)
        {
        case WM_INITDIALOG:
            hwndDrvPath = GetDlgItem(hwnd, IDC_EDIT_DRVFULLPATH) ;
            hwndStatus  = GetDlgItem(hwnd, IDC_STATIC_STATUS) ;
            DragAcceptFiles(hwnd, TRUE) ; //使窗口支持文件拖拽的功能
            PopFileInitialize (hwnd, &ofn) ; //初始化打开文件对话框
            return FALSE ;

        case WM_DROPFILES:  //当文件拖放到窗口时
            hDrop = (HDROP)wParam ;
            DragQueryFile(hDrop, 0, szFullPath, MAX_PATH) ; //这里我们只支持一个文件
            SetDlgItemText(hwnd, IDC_EDIT_DRVFULLPATH, szFullPath) ;
            return 0 ;
                
        case WM_COMMAND:
            switch (LOWORD (wParam)) //LOWORD(wParam)是控件的ID 高字组是通知码
            {
            case IDCANCEL:
                EndDialog (hwnd, 0) ;
                break ;

            case IDC_BTN_BROWSE: //浏览按钮
                if(PopFileOpenDlg(hwnd, &ofn, szFullPath, szTitle))
                        SetDlgItemText(hwnd, IDC_EDIT_DRVFULLPATH, szFullPath) ;
                break ;

            case IDC_BTN_INSTALL: //安装按钮
            case IDC_BTN_START:   //开始按钮
            case IDC_BTN_STOP:    //停止按钮
            case IDC_BTN_REMOVE:  //移除按钮
                GetDlgItemText(hwnd, IDC_EDIT_DRVFULLPATH, szFullPath, MAX_PATH) ;
                pStr = szFullPath + lstrlen(szFullPath) ;
                while(*pStr != '\\' && pStr-szFullPath!=0)
                        pStr-- ;
                if(pStr != szFullPath)
                        pStr++ ;
                //_tcsncpy(szTitle, pStr, MAX_PATH) ;
                for(iOperaType=0; *(pStr+iOperaType); iOperaType++)
                        szTitle[iOperaType] = *(pStr+iOperaType) ;
                szTitle[iOperaType] = '\0' ;

  //这里可以不用以下这么写，可以ID相减得出操作类型(只要设定好ID)
                switch(LOWORD (wParam))
                {
                case IDC_BTN_INSTALL:
                    iOperaType = 0 ;
                    break ;

                case IDC_BTN_START:
                    iOperaType = 1 ;
                    break ;

                case IDC_BTN_STOP:
                    iOperaType = 2 ;
                    break ;

                case IDC_BTN_REMOVE:
                    iOperaType = 3 ;
                    break ;
                }
                operaType(szFullPath, szTitle, iOperaType) ;
                break ;
            }
            return TRUE ;
                
            case WM_SYSCOMMAND:
                switch (LOWORD (wParam))
                {
                case SC_CLOSE:
                    EndDialog (hwnd, 0) ;                   
                    return TRUE ;
                }
                break ;
        }
        return FALSE ;
}

//szFullPath完整路径、szName服务名、iType操作类型(安装、运行、停止、移除)
BOOL operaType(TCHAR *szFullPath, TCHAR *szName, int iType)
{
        SC_HANDLE shOSCM = NULL, shCS = NULL ;
        SERVICE_STATUS ss ;
        DWORD dwErrorCode = 0 ;
        BOOL bSuccess = FALSE ;

        shOSCM = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
        if( !shOSCM )
        {
				ReportError(hwndStatus, TEXT("OpenSCManager!")) ;
                return FALSE ;
        }

        if(iType) //若操作类型不是"安装服务"
        {
                shCS = OpenService(shOSCM, szName, SERVICE_ALL_ACCESS) ;
                if( !shCS )
                {
                        ReportError(hwndStatus, TEXT("OpenService!")) ;
                        CloseServiceHandle(shOSCM); //关闭服务句柄
                        shOSCM = NULL ;
                        return FALSE;
                }
        }

        switch(iType)
        {
        case 0: //安装服务
            shCS = CreateService(shOSCM, szName, szName,
                    SERVICE_ALL_ACCESS, SERVICE_KERNEL_DRIVER,
                    SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL,
                    szFullPath, NULL, NULL, NULL, NULL, NULL) ;

            if( !shCS )
            {
                    ReportError(hwndStatus, TEXT("CreateService！")) ;
                    bSuccess = FALSE ;
                    break ;
            }
            bSuccess = TRUE ;
            SetWindowText(hwndStatus, TEXT("安装服务成功！")) ;
            break ;

        case 1: //运行服务
            if(StartService(shCS, 0, NULL))
                    SetWindowText(hwndStatus, TEXT("运行指定服务成功！")) ;
            else
            {
					ReportError(hwndStatus, TEXT("Start Service！")) ;
                    bSuccess = FALSE ;
                    break ;
            }
            bSuccess = TRUE ;
            break ;

        case 2: //停止服务
            if(!ControlService(shCS, SERVICE_CONTROL_STOP, &ss))
            {
                    ReportError(hwndStatus, TEXT("StopService！")) ;
                    bSuccess = FALSE ;
                    break ;
            }
            SetWindowText(hwndStatus, TEXT("成功停止服务！")) ;
            bSuccess = TRUE ;
            break ;

        case 3: //移除服务
            if(!DeleteService(shCS))
            {
                    ReportError(hwndStatus, TEXT("DeleteService！")) ;
                    bSuccess = FALSE ;
                    break ;
            }
            SetWindowText(hwndStatus, TEXT("成功移除服务！")) ;
            bSuccess = TRUE ;
            break ;
            
        default:
            break ;
        }

        if(shCS)
        {
                CloseServiceHandle(shCS) ;
                shCS = NULL ;
        }
        if(shOSCM)
        {
                CloseServiceHandle(shOSCM) ;
                shOSCM = NULL ;
        }

        return bSuccess ;
}

void PopFileInitialize (HWND hwnd, OPENFILENAME *pOfn)
{
        static TCHAR szFilter[] = TEXT ("Sys Files (*.sys)\0*.sys\0")  \
                TEXT ("All Files (*.*)\0*.*\0\0") ;
        
        pOfn->lStructSize       = sizeof (OPENFILENAME) ;
        pOfn->hwndOwner         = hwnd ;
        pOfn->hInstance         = NULL ;
        pOfn->lpstrFilter       = szFilter ;
        pOfn->lpstrCustomFilter = NULL ;
        pOfn->nMaxCustFilter    = 0 ;
        pOfn->nFilterIndex      = 0 ;
        pOfn->lpstrFile         = NULL ;          // Set in Open and Close functions
        pOfn->nMaxFile          = MAX_PATH ;
        pOfn->lpstrFileTitle    = NULL ;          // Set in Open and Close functions
        pOfn->nMaxFileTitle     = MAX_PATH ;
        pOfn->lpstrInitialDir   = NULL ;
        pOfn->lpstrTitle        = NULL ;
        pOfn->Flags             = 0 ;             // Set in Open and Close functions
        pOfn->nFileOffset       = 0 ;
        pOfn->nFileExtension    = 0 ;
        pOfn->lpstrDefExt       = TEXT ("sys") ;
        pOfn->lCustData         = 0L ;
        pOfn->lpfnHook          = NULL ;
        pOfn->lpTemplateName    = NULL ;
}

BOOL PopFileOpenDlg (HWND hwnd, OPENFILENAME *pOfn, PTSTR pstrFileName, PTSTR pstrTitleName)
{
        pOfn->hwndOwner         = hwnd ;
        pOfn->lpstrFile         = pstrFileName ;
        pOfn->lpstrFileTitle    = pstrTitleName ;
        pOfn->Flags             = OFN_FILEMUSTEXIST ; //文件必须存在的
        
        return GetOpenFileName (pOfn) ;
}

