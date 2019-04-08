
// ChatClientDlg.cpp : implementation file
//

#include "stdafx.h"
#include "ChatClient.h"
#include "ChatClientDlg.h"
#include "afxdialogex.h"

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <winsock2.h>
//#include <netinet/in.h>
#include <string.h>
#include <signal.h>
#include <io.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define MAXLINE 4096
#define SERVER_PORT 9939

char buf[MAXLINE];

// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CChatClientDlg dialog



CChatClientDlg::CChatClientDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_CHATCLIENT_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CChatClientDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CChatClientDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDOK, &CChatClientDlg::OnBnClickedOk)
END_MESSAGE_MAP()


// CChatClientDlg message handlers

BOOL CChatClientDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CChatClientDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CChatClientDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CChatClientDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

/*---------------------------------------------------*/

int Hstrlen(char *s)
{
	if (s == NULL) return 0;
	int res = 0;
	while (s[res] != 0)
	{
		res++;
	}
	return res;
}

LPCTSTR HcharStringToLPCTSTR(char *tempchar)
{
	int n = Hstrlen(tempchar);
	wchar_t* temp = (wchar_t*)malloc((n + 1) * sizeof(wchar_t));
	for (int i = 0; i < n; i++)
	{
		temp[i] = (wchar_t)tempchar[i];
	}
	temp[n] = 0;
	return (LPCTSTR)temp;
}

/*-------------------------------------------------------------*/

void CChatClientDlg::OnBnClickedOk()
{
	// TODO: Add your control notification handler code here

	struct sockaddr_in client_socket;
	struct sockaddr_in server_socket;
	int client_sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	server_socket.sin_family = AF_INET;
	server_socket.sin_port = htons(SERVER_PORT);
	server_socket.sin_addr.s_addr = inet_addr("127.0.0.1");
	CEdit* result = (CEdit*)GetDlgItem(IDC_EDIT1);
	if (connect(client_sock_fd, (struct sockaddr *) &server_socket, sizeof(server_socket)) < 0)
	{
		//printf("Error in connecting to server\n");
		result->SetWindowTextW(HcharStringToLPCTSTR("Error in connecting to server"));
		return;
	}
	else
		result->SetWindowTextW(HcharStringToLPCTSTR("Connected to server"));
		//printf("connected to server\n");

	int n;
	CString temp_CString;
	CEdit* pEdit = (CEdit*)GetDlgItem(IDC_EDIT2);
	pEdit->GetWindowTextW(temp_CString);
	CT2A temp_ascii2(temp_CString);
	char *inp = (char*)temp_ascii2.m_psz;
	send(client_sock_fd, inp, Hstrlen(inp), 0);

	if ((n = recv(client_sock_fd, buf, MAXLINE, 0)) == 0) {
		//error: server terminated prematurely
		result = (CEdit*)GetDlgItem(IDC_EDIT3);
		result->SetWindowTextW(HcharStringToLPCTSTR("Receive error"));
	}
	else {
		buf[n] = 0;
		result = (CEdit*)GetDlgItem(IDC_EDIT3);
		result->SetWindowTextW(HcharStringToLPCTSTR(buf));
	}
	closesocket(client_sock_fd);
	//CDialogEx::OnOK();
}
