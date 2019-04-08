
// ChatServerDlg.cpp : implementation file
//

#include "stdafx.h"
#include "ChatServer.h"
#include "ChatServerDlg.h"
#include "afxdialogex.h"

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <winsock2.h>
//#include <netinet/in.h>
#include <string.h>
#include <signal.h>
#include <io.h>

//#include <sqlite3.h> 

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define ERROR_REQUEST_VALUE "Error request value"
#define ERROR_USER_EXISTED "User existed"
#define ERROR_USER_NOT_EXISTED "User not existed"
#define ERROR_LOGIN_INFORMATION "Invalid login information"
#define ERROR_TOKEN "Invalid token"
#define REGISTER_FAIL "Register fail"
#define REGISTER_SUCCESS "Register success"
#define LOGIN_FAIL "Login fail"
#define LOGIN_SUCCESS "Login success"
#define SEND_MESSAGE_FAIL "Send message fail"
#define SEND_MESSAGE_SUCCESS "Send message success"

#define JSON 0
#define STRING 1

#define DEFAULT_IP "0.0.0.0"
#define DEFAULT_PORT "9939"
#define START_MSG "Server running...waiting for connections."
#define LISTENQ 8 /*maximum number of client connections */
#define MAXLINE 4096 /*max text line length*/
#define MAXUSER 93
#define DBNAME "chatdatabase.db"

int listenfd, connfd;
int childpid, pid;
socklen_t clilen;
char buf[MAXLINE+1];
struct sockaddr_in cliaddr, servaddr;
char *client_ip;
int client_port;
int nuser = 0;
char *username[MAXUSER], *password[MAXUSER];
int nclient = 0;
char base64char[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/*--------------------------------------*/

typedef struct LISTENDATA {
	CListCtrl *result = NULL;
	int listenfd;
} LISTENDATA, *PLISTENDATA;

/*--------------------------------------*/
// utils.h

char *Hitoa(long long x)
{
	int n = 0;
	long long tempx = x;
	do
	{
		n++;
		tempx /= 10;
	} while (tempx > 0);
	char *res = (char*)malloc(n + 1);
	for (int i = n - 1; i >= 0; i--)
	{
		res[i] = x % 10 + '0';
		x /= 10;
	}
	res[n] = 0;
	return res;
}

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

char find_base64char(char inp)
{
	if (inp >= 'A' && inp <= 'Z') return inp - 'A';
	if (inp >= 'a' && inp <= 'z') return inp - 'a' + 26;
	if (inp >= '0' && inp <= '9') return inp - '0' + 52;
	if (inp == '+') return 62;
	if (inp == '/') return 63;
	if (inp == '=') return 0;
	return -1;
}

char *b64encode(char *inp)
{
	int n = Hstrlen(inp), i;
	char *res = (char*)malloc((n / 3 + 1) * 4 + 1);
	int reslen = 0;
	for (i = 0; i <= n - 3; i += 3)
	{
		res[reslen++] = base64char[inp[i] >> 2];
		res[reslen++] = base64char[(inp[i] & 3) << 4 | (inp[i + 1] >> 4)];
		res[reslen++] = base64char[(inp[i + 1] & 15) << 2 | (inp[i + 2] >> 6)];
		res[reslen++] = base64char[inp[i + 2] & 63];
	}
	if (n % 3 == 1)
	{
		res[reslen++] = base64char[inp[i] >> 2];
		res[reslen++] = base64char[(inp[i] & 3) << 4];
		res[reslen++] = '=';
		res[reslen++] = '=';
	}
	else if (n % 3 == 2)
	{
		res[reslen++] = base64char[inp[i] >> 2];
		res[reslen++] = base64char[(inp[i] & 3) << 4 | (inp[i + 1] >> 4)];
		res[reslen++] = base64char[(inp[i + 1] & 15) << 2];
		res[reslen++] = '=';
	}
	res[reslen] = 0;
	return res;
}

char *b64decode(char *inp)
{
	int n = Hstrlen(inp), i;
	if (n % 4 != 0) return NULL;
	char *res = (char*)malloc((n / 4) * 3 + 1);
	int reslen = 0;
	for (i = 0; i < n; i += 4)
	{
		res[reslen++] = find_base64char(inp[i]) << 2 | find_base64char(inp[i + 1]) >> 6;
		res[reslen++] = (find_base64char(inp[i + 1]) & 15) << 4 | (find_base64char(inp[i + 2]) >> 2);
		res[reslen++] = (find_base64char(inp[i + 2]) & 3) << 6 | find_base64char(inp[i + 3]);
	}
	res[reslen] = 0;
	return res;
}

char *get_field_json(char *json, char *field)
{
	int i = 0;
	int n = strlen(json);
	int m = strlen(field);
	int json_level = 0;
	int is_in_string = 0;
	int start_str = 0;
	int end_str = 0;
	int is_return_value = 0;
	int is_field = 0;
	int is_value = 0;
	for (i = 0; i < n; i++)
	{
		if (is_in_string)
		{
			if (json[i] == '"')
			{
				is_in_string = 0;
				if (json_level == 1)
				{
					end_str = i;
					if (is_return_value)
					{
						n = end_str - start_str;
						char *res = (char*)malloc(end_str - start_str);
						for (int j = 0; j < n; j++)
						{
							res[j] = json[j + start_str];
						}
						res[n] = 0;
						return res;
					}
					else if (is_field)
					{
						if (end_str - start_str == m)
						{
							if (strncmp(field, json + start_str, m) == 0)
							{
								is_return_value = 99;
							}
						}
					}
				}
			}
		}
		else
		{
			if (json[i] == '"')
			{
				is_in_string = 93;
				if (json_level == 1) start_str = i + 1;
			}
			else if (json[i] == ':')
			{
				if (json_level == 1)
				{
					if (is_field)
					{
						is_field = 0;
						is_value = 99;
					}
					else return NULL; // invalid json
				}
			}
			else if (json[i] == ',')
			{
				if (json_level == 1)
				{
					if (is_value)
					{
						is_value = 0;
						is_field = 99;
					}
					else return NULL; // invalid json
				}
			}
			else if (json[i] == '{')
			{
				if (json_level == 1)
				{
					start_str = i;
				}
				else if (json_level == 0) is_field = 99;
				json_level++;
			}
			else if (json[i] == '}')
			{
				json_level--;
				if (json_level == 1)
				{
					end_str = i + 1;
					if (is_return_value)
					{
						n = end_str - start_str;
						char *res = (char*)malloc(end_str - start_str);
						for (int j = 0; j < n; j++)
						{
							res[j] = json[j + start_str];
						}
						res[n] = 0;
						return res;
					}
				}
			}
		}
	}
	return NULL;
}

char *add_to_json(char *original_json, char *field, char *value, int value_type)
{
	int original_json_len = Hstrlen(original_json);
	int field_len = Hstrlen(field);
	int value_len = Hstrlen(value);
	if (original_json == NULL)
	{
		original_json = (char*)malloc(field_len + value_len + 10);
		memset(original_json, 0, sizeof original_json);
		strcat(original_json, "{\"");
		strcat(original_json, field);
		strcat(original_json, "\": ");
		if (value_type == STRING) strcat(original_json, "\"");
		strcat(original_json, value);
		if (value_type == STRING) strcat(original_json, "\"");
		strcat(original_json, "}");
		return original_json;
	}
	original_json = (char*)realloc(original_json, original_json_len + field_len + value_len + 10);
	for (int i = original_json_len - 1; i >= 0; i--)
	{
		if (original_json[i] == '}')
		{
			original_json[i] = ',';
			strcat(original_json, " \"");
			strcat(original_json, field);
			strcat(original_json, "\": ");
			if (value_type == STRING) strcat(original_json, "\"");
			strcat(original_json, value);
			if (value_type == STRING) strcat(original_json, "\"");
			strcat(original_json, "}");
			return original_json;
		}
		else original_json[i] = 0;
	}
	return original_json;
}

/*-----------------------------------------------------*/

void add_user(char *username_, char *password_)
{
	username[nuser] = _strdup(username_);
	password[nuser] = _strdup(password_);
	nuser++;
}

void add_user_to_db(char *username, char *password)
{
	FILE *f = fopen(DBNAME, "a");
	fprintf(f, "%s\n", username);
	fprintf(f, "%s\n", password);
	fclose(f);
}

void import_user()
{
	nuser = 0;
	FILE *f = fopen(DBNAME, "r");
	if (f == NULL) return;
	char user[MAXLINE], pass[MAXLINE];
	user[0] = 0;
	pass[0] = 0;
	while (99)
	{
		fgets(user, MAXLINE, f);
		if (user[0] == 0) break;
		fgets(pass, MAXLINE, f);
		for (int i = 0; i < MAXLINE; i++)
			if (user[i] == '\n')
			{
				user[i] = 0;
				break;
			}
			else if (user[i] == 0) break;
		for (int i = 0; i < MAXLINE; i++)
			if (pass[i] == '\n')
			{
				pass[i] = 0;
				break;
			}
			else if (pass[i] == 0) break;
		add_user(user, pass);
	}
	fclose(f);
}

int chat_check_exist_user(char *user)
{
	for (int i = 0; i < nuser; i++)
	{
		FILE *f = fopen("checktest.txt", "w");
		fprintf(f, "%s\n%s\n", user, username[i]);
		fclose(f);
		if (strcmp(user, username[i]) == 0) return i; // return id of user
	}
	return -1;
}

int chat_check_login(char *user, char *pass)
{
	for (int i = 0; i < nuser; i++)
	{
		if (strcmp(user, username[i]) == 0 && strcmp(pass, password[i]) == 0)
			return i; // return id of user
	}
	return -1;
}

char *chat_register(char *json_value)
{
	//FILE *f = fopen("registertest.txt", "w");
	char *user = get_field_json(json_value, "username");
	char *pass = get_field_json(json_value, "password");
	char *response = NULL;
	char *response_value = NULL;
	response = add_to_json(response, "request", "response_register", STRING);
	if (user == NULL || pass == NULL)
	{
		response_value = add_to_json(response_value, "response", REGISTER_FAIL, STRING);
		response_value = add_to_json(response_value, "error_message", ERROR_REQUEST_VALUE, STRING);
		response = add_to_json(response, "value", response_value, JSON);
		return response;
	}
	if (chat_check_exist_user(user) == -1)
	{
		add_user(user, pass);
		add_user_to_db(user, pass);
		response_value = add_to_json(response_value, "response", REGISTER_SUCCESS, STRING);
		response = add_to_json(response, "value", response_value, JSON);
		//fprintf(f, "%s\n%s\n%s\n", response, response_value, pass);
		//fclose(f);
		return response;
	}
	response_value = add_to_json(response_value, "response", REGISTER_FAIL, STRING);
	response_value = add_to_json(response_value, "error_message", ERROR_USER_EXISTED, STRING);
	response = add_to_json(response, "value", response_value, JSON);
	return response;
}

char *chat_login(char *json_value)
{
	char *user = get_field_json(json_value, "username");
	char *pass = get_field_json(json_value, "password");
	char *response = NULL;
	char *response_value = NULL;
	response = add_to_json(response, "request", "response_login", STRING);
	if (user == NULL || pass == NULL)
	{
		response_value = add_to_json(response_value, "response", LOGIN_FAIL, STRING);
		response_value = add_to_json(response_value, "error_message", ERROR_REQUEST_VALUE, STRING);
		response = add_to_json(response, "value", response_value, JSON);
		return response;
	}
	int id = -1;
	if ((id = chat_check_login(user, pass)) != -1)
	{
		response_value = add_to_json(response_value, "response", LOGIN_SUCCESS, STRING);
		response_value = add_to_json(response_value, "login_token", Hitoa(id), STRING);
		response = add_to_json(response, "value", response_value, JSON);
		return response;
	}
	response_value = add_to_json(response_value, "response", LOGIN_FAIL, STRING);
	response_value = add_to_json(response_value, "error_message", ERROR_LOGIN_INFORMATION, STRING);
	response = add_to_json(response, "value", response_value, JSON);
	return response;
}

int chat_validate_token(char *login_token, char *user_name)
{
	// just a simple validation
	for (int i = 0; i < strlen(login_token);i++)
	{
		if (login_token[i] < '0' || login_token[i] > '9') return 0;
	}
	int user_id = atoi(login_token);
	if (user_id < 0 || user_id >= nuser) return 0;
	if (strcmp(login_token, username[user_id]) == 0) return 99;
	else return 0;
}

int chat_add_message_to_db(char *sender, char *receiver, char *message, char *timestamp)
{
	return 0;
}

char *chat_send_message(char *json_value)
{
	char *login_token = get_field_json(json_value, "login_token");
	char *sender_name = get_field_json(json_value, "sender_name");
	char *receiver_name = get_field_json(json_value, "receiver_name");
	char *message = get_field_json(json_value, "message");
	char *timestamp = get_field_json(json_value, "timestamp");
	char *response = NULL;
	char *response_value = NULL;
	response = add_to_json(response, "request", "response_send_message", STRING);
	if (login_token == NULL || sender_name == NULL || receiver_name == NULL || message == NULL || timestamp == NULL)
	{
		response_value = add_to_json(response_value, "response", SEND_MESSAGE_FAIL, STRING);
		response_value = add_to_json(response_value, "error_message", ERROR_REQUEST_VALUE, STRING);
		response = add_to_json(response, "value", response_value, JSON);
		return response;
	}
	if (chat_check_exist_user(sender_name) || chat_check_exist_user(receiver_name))
	{
		response_value = add_to_json(response_value, "response", SEND_MESSAGE_FAIL, STRING);
		response_value = add_to_json(response_value, "error_message", ERROR_USER_NOT_EXISTED, STRING);
		response = add_to_json(response, "value", response_value, JSON);
		return response;
	}
	if (chat_validate_token(login_token, sender_name) == 0)
	{
		response_value = add_to_json(response_value, "response", SEND_MESSAGE_FAIL, STRING);
		response_value = add_to_json(response_value, "error_message", ERROR_TOKEN, STRING);
		response = add_to_json(response, "value", response_value, JSON);
		return response;
	}
	chat_add_message_to_db(sender_name, receiver_name, message, timestamp);
	response_value = add_to_json(response_value, "response", SEND_MESSAGE_SUCCESS, STRING);
	response = add_to_json(response, "value", response_value, JSON);
	return response;
}

char *request_process(char *request_json)
{
	char *request = get_field_json(request_json, "request");
	if (request == NULL) return ERROR_REQUEST_VALUE;
	char *json_value = get_field_json(request_json, "value");
	if (json_value == NULL) return ERROR_REQUEST_VALUE;
	char *response = NULL;
	if (strcmp(request, "register") == 0)
	{
		response = chat_register(json_value);
		return response;
	}
	else if (strcmp(request, "login") == 0)
	{
		response = chat_login(json_value);
		return response;
	}
	else if (strcmp(request, "send_message") == 0)
	{
		response = chat_send_message(json_value);
		return response;
	}
	return ERROR_REQUEST_VALUE;
}

/*---------------------------------------------*/

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


// CChatServerDlg dialog



CChatServerDlg::CChatServerDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_CHATSERVER_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CChatServerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT1, serverIP);
}

BEGIN_MESSAGE_MAP(CChatServerDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDOK, &CChatServerDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CChatServerDlg::OnBnClickedCancel)
END_MESSAGE_MAP()


// CChatServerDlg message handlers

BOOL CChatServerDlg::OnInitDialog()
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

	SetDlgItemTextW(IDC_EDIT1, HcharStringToLPCTSTR(DEFAULT_IP));	// set default IP
	SetDlgItemTextW(IDC_EDIT2, HcharStringToLPCTSTR(DEFAULT_PORT));	// set default port

	CListCtrl* listctrl = (CListCtrl*)GetDlgItem(IDC_LIST1);
	listctrl->InsertColumn(0, HcharStringToLPCTSTR("Client IP"), -1, 100, -1);
	listctrl->InsertColumn(1, HcharStringToLPCTSTR("Client Port"), 0, 70, -1);
	listctrl->InsertColumn(2, HcharStringToLPCTSTR("Message"), 0, 250, -1);
	listctrl->SetExtendedStyle(listctrl->GetExtendedStyle() | LVS_EX_FULLROWSELECT);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CChatServerDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CChatServerDlg::OnPaint()
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
HCURSOR CChatServerDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

DWORD WINAPI listenthread(LPVOID lpParam)
{
	LISTENDATA *listendata = (PLISTENDATA)lpParam;
	int listenfd = listendata->listenfd;
	int buflen;
	char buf[MAXLINE + 1];
	char *response = NULL;
	CListCtrl *result = listendata->result;
	while (99)
	{
		int connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &clilen);
		char *client_ip = inet_ntoa(cliaddr.sin_addr);
		int client_port = htons(cliaddr.sin_port);
		while ((buflen = recv(connfd, buf, MAXLINE, 0)) > 0) {
			int t = result->InsertItem(nclient, HcharStringToLPCTSTR(client_ip));
			result->SetItemText(t, 1, HcharStringToLPCTSTR(Hitoa(client_port)));
			nclient++;
			buf[buflen] = 0;
			result->SetItemText(t, 2, HcharStringToLPCTSTR(buf));
			response = request_process(buf);
			send(connfd, response, Hstrlen(response), 0);
			//send(connfd, response, Hstrlen(response), 0);
			//printf("%s", "String received from and resent to the client:");
			//puts(buf);
			//send(connfd, buf, n, 0);
		}
		closesocket(connfd);
	}
	closesocket(listenfd);
}

void CChatServerDlg::OnBnClickedOk()
{
	// TODO: Add your control notification handler code here

	CString temp_CString;
	CEdit* pEdit;

	char *ip_value, *port_value;
	
	pEdit = (CEdit*)GetDlgItem(IDC_EDIT1);
	pEdit->GetWindowTextW(temp_CString);
	CT2A temp_ascii(temp_CString);
	ip_value = temp_ascii.m_psz;

	pEdit = (CEdit*)GetDlgItem(IDC_EDIT2);
	pEdit->GetWindowTextW(temp_CString);
	CT2A temp_ascii2(temp_CString);
	port_value = temp_ascii2.m_psz;

	CListCtrl *result = (CListCtrl*)GetDlgItem(IDC_LIST1);

	//creation of the socket
	listenfd = socket(AF_INET, SOCK_STREAM, 0);

	//preparation of the socket address 
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(atoi(port_value));

	bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr));

	listen(listenfd, LISTENQ);
	
	SetDlgItemTextW(IDC_EDIT3, HcharStringToLPCTSTR(START_MSG));

	clilen = sizeof(cliaddr);

	LISTENDATA listendata;
	listendata.result = result;
	listendata.listenfd = listenfd;

	HANDLE thread = CreateThread(NULL, 0, listenthread, &listendata, 0, NULL);
	/*
	//while (99)
	{
		connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &clilen);
		client_ip = inet_ntoa(cliaddr.sin_addr);
		client_port = htons(cliaddr.sin_port);
		int t = result->InsertItem(nclient, HcharStringToLPCTSTR(client_ip));
		result->SetItemText(t, 1, HcharStringToLPCTSTR(Hitoa(client_port)));
		closesocket(connfd);
		nclient++;
	}*/
	//closesocket(listenfd);
	//CDialogEx::OnOK();
}


void CChatServerDlg::OnBnClickedCancel()
{
	// TODO: Add your control notification handler code here
	closesocket(listenfd);
	CDialogEx::OnCancel();
}
