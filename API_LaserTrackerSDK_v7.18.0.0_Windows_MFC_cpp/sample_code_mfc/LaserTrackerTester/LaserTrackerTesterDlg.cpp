
// LaserTrackerTesterDlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "LaserTrackerTester.h"
#include "LaserTrackerTesterDlg.h"
#include "afxdialogex.h"

#include <fstream>
#include <chrono>
#include <ctime>
#include <string>
#include <iomanip>



#ifdef _DEBUG
#define new DEBUG_NEW
#endif

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


// LaserTrackerTesterDlg dialog



LaserTrackerTesterDlg::LaserTrackerTesterDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_LASERTRACKERTESTER_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_CurrentTask = api::lt::TaskMode::Idle;
	m_iTimePeriod = 1;
	m_fDistance = 1.0f;
	m_uiAverageTime = 1000;
	m_uiAverageTimeMultiSMR = 1000;
	m_uiAvergeTimeStablePointSMR = 1000;
	m_fEstimatedDistance = 2000;
	m_fEstimatedRadius = 1000;
	m_uiTimeout = 40000;
	m_Device = nullptr;
    m_bTTLChecked = false;
	m_AccModel = api::lt::AccessoryModel::None;
	m_fAz = 0.0f;
	m_fEl = 0.0f;
	m_fX = 0.0f;
	m_fY = 0.0f;
	m_fZ = 0.0f;
	m_iPointToNotify = 0;
	//m_dlgVideoStream = nullptr;
}

LaserTrackerTesterDlg::~LaserTrackerTesterDlg()
{
}

void LaserTrackerTesterDlg::DoDataExchange(CDataExchange* pDX)
{
	DDX_Text(pDX, IDC_EDIT_X, m_fX);
	DDX_Text(pDX, IDC_EDIT_Y, m_fY);
	DDX_Text(pDX, IDC_EDIT_Z, m_fZ);
	DDX_Text(pDX, IDC_EDIT_AVERAGE_TIME, m_uiAverageTime);
	DDX_Check(pDX, IDC_POINTTONOTIFY, m_iPointToNotify);
	DDX_Text(pDX, IDC_EDIT_TIMEPERIOD, m_iTimePeriod);
	DDX_Text(pDX, IDC_EDIT_DISTANCE, m_fDistance);
	DDX_Text(pDX, IDC_EDIT_AVERAGE_TIME_MULTISMR, m_uiAverageTimeMultiSMR);
	DDX_Text(pDX, IDC_EDIT_ESTDISTANCE, m_fEstimatedDistance);
	DDX_Text(pDX, IDC_EDIT_ESTRADIUS, m_fEstimatedRadius);
	DDX_Text(pDX, IDC_EDIT_TIMEOUT, m_uiTimeout);
	CDialogEx::DoDataExchange(pDX);
}

void LaserTrackerTesterDlg::WorkerProc(LaserTrackerTesterDlg* p) 
{
	bool run = true;
	while (run) 
	{
		std::unique_lock<std::mutex> lock(p->m_mTaskMtx);
		p->m_cTaskCv.wait(lock);
		if (p->m_sTaskResult.error_code == api::lt::ErrorType::Success) 
		{
			switch (p->m_sTaskResult.task) 
			{ 
				case api::lt::TaskMode::Idle:
					run = false;
					break;

				case api::lt::TaskMode::VirtualLevel: 
				{           
					float matrix[9];
					auto ret = p->m_Device->GetVirtualLevelMatrix(matrix);
					if (ret == api::lt::ErrorType::Success) 
					{
						std::ofstream vl("VirtualLevel.txt");
						if (vl.is_open()) 
						{
							vl << std::fixed << std::setprecision(6);
							for (int i = 0; i < 3; ++i)
							{
								for (int j = 0; j < 3; ++j)
								{
									vl << std::setw(12) << std::left <<  matrix[i * 3 + j] << "\t";
								}
								vl << "\n";
							}
							vl.close();
						}
					}
				}
				break;

				default:
					break;
			}
			CString str;
			str.Format(_T("%s: Task finished"), p->GetCurrentDateTime().c_str());
			p->GetDlgItem(IDC_EDIT_STATUS)->SetWindowText(str);
		} 
		else 
		{
			char error_string[api::lt::MAX_ERROR_STRING_LENGTH];
			p->m_Device->GetErrorMessage(p->m_sTaskResult.error_code, error_string);
			std::string sn(error_string);
			std::wstring w_error_string(sn.begin(), sn.end());
			CString str;
			str.Format(_T("%s: Task failed: %d\r\nError description: %s"), p->GetCurrentDateTime().c_str(), (int)p->m_sTaskResult.task, w_error_string.c_str());
			p->GetDlgItem(IDC_EDIT_STATUS)->SetWindowText(str);
		}
    }
}

BEGIN_MESSAGE_MAP(LaserTrackerTesterDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_CONNECT, &LaserTrackerTesterDlg::OnBnClickedButtonConnect)
	ON_BN_CLICKED(IDC_BUTTON_DISCONNECT, &LaserTrackerTesterDlg::OnBnClickedButtonDisconnect)
	ON_BN_CLICKED(IDC_BUTTON_VIRTUALLEVEL, &LaserTrackerTesterDlg::OnBnClickedButtonVirtuallevel)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_BUTTON_HOME, &LaserTrackerTesterDlg::OnBnClickedButtonHome)
	ON_BN_CLICKED(IDC_BUTTON_POINTTOCARTESIAN, &LaserTrackerTesterDlg::OnBnClickedButtonPointtocartesian)
	ON_BN_CLICKED(IDC_BUTTON_POINTTO_SPHERICAL, &LaserTrackerTesterDlg::OnBnClickedButtonPointtoSpherical)
	ON_BN_CLICKED(IDC_CHECK_ENABLECAMERA_SEARCH, &LaserTrackerTesterDlg::OnBnClickedCheckEnablecameraSearch)
	ON_BN_CLICKED(IDC_ABORT_PROCEDURE, &LaserTrackerTesterDlg::OnBnClickedAbortProcedure)
	ON_BN_CLICKED(IDC_BUTTON_STATIC_DATACOLLECTION, &LaserTrackerTesterDlg::OnBnClickedButtonStaticDatacollection)
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDC_RELAXMOTORS, &LaserTrackerTesterDlg::OnBnClickedRelaxmotors)
	ON_BN_CLICKED(IDC_TRACK, &LaserTrackerTesterDlg::OnBnClickedTrack)
    ON_BN_CLICKED(IDC_BUTTON_OPENIVISION_WINDOW, &LaserTrackerTesterDlg::OnBnClickedButtonOpenivisionWindow)
    ON_BN_CLICKED(IDC_BUTTON_DYNAMICTEMPORAL, &LaserTrackerTesterDlg::OnBnClickedButtonDynamictemporal)
    ON_BN_CLICKED(IDC_BUTTON_STOPMEASUREMENT, &LaserTrackerTesterDlg::OnBnClickedButtonStopmeasurement)
    ON_BN_CLICKED(IDC_BUTTON_SPATIALDYNAMIC, &LaserTrackerTesterDlg::OnBnClickedButtonSpatialdynamic)
    ON_BN_CLICKED(IDC_BUTTON_MULTISMR_DATACOLLECTION, &LaserTrackerTesterDlg::OnBnClickedButtonMultismrDatacollection)
    ON_BN_CLICKED(IDC_BUTTON_SPIRALSEARCH, &LaserTrackerTesterDlg::OnBnClickedButtonSpiralsearch)
    ON_BN_CLICKED(IDC_BUTTON_POINTTOCARTESIANSEARCH,
                  &LaserTrackerTesterDlg::OnBnClickedButtonPointtocartesiansearch)
    ON_BN_CLICKED(IDC_BUTTON_TTLMEASUREMENT,
                  &LaserTrackerTesterDlg::OnBnClickedButtonTtlmeasurement)
	ON_BN_CLICKED(IDC_BUTTON_SINGLEPOINTPROBEDATACOLLECTION, &LaserTrackerTesterDlg::OnBnClickedButtonSinglepointprobedatacollection)
	ON_BN_CLICKED(IDC_BUTTON_DYNAMICPROBEDATACOLLECTION, &LaserTrackerTesterDlg::OnBnClickedButtonDynamicprobedatacollection)
	ON_BN_CLICKED(IDC_BUTTON_STOPPROBEMEASUREMENT, &LaserTrackerTesterDlg::OnBnClickedButtonStopprobemeasurement)
	ON_BN_CLICKED(IDC_BUTTON_OPENPROBEIFCDIALOG, &LaserTrackerTesterDlg::OnBnClickedButtonOpenprobeifcdialog)
	ON_BN_CLICKED(IDC_BUTTON_OPENVIDEOSTREAMING, &LaserTrackerTesterDlg::OnBnClickedButtonOpenvideostreaming)
END_MESSAGE_MAP()


// LaserTrackerTesterDlg message handlers

BOOL LaserTrackerTesterDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
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
	m_Device = nullptr;
	m_fX = m_fY = m_fZ = 0.0f;
	m_uiAverageTime = 1000;
	m_iPointToNotify = 0;
	m_fAz = m_fEl = 0.0f;
	((CIPAddressCtrl*)GetDlgItem(IDC_IPADDRESS))->SetAddress(192, 168, 0, 168);
	CheckRadioButton(IDC_RADIO_ONEHALFINCH, IDC_RADIO_SEVENEIGHTHINCH, IDC_RADIO_ONEHALFINCH);
	m_Device = new api::lt::LaserTrackerInterface();
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void LaserTrackerTesterDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void LaserTrackerTesterDlg::OnPaint()
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
HCURSOR LaserTrackerTesterDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void LaserTrackerTesterDlg::TaskCallbackFunction(const api::lt::TaskResult& task_result,
                                                 void* callback_parameter) 
{
	if (!callback_parameter) 
	{
		return;
	}
	// aquire task mutex and set task result to task status
	auto obj = static_cast<LaserTrackerTesterDlg*>(callback_parameter);
	{
		std::lock_guard<std::mutex> lock(obj->m_mTaskMtx);
		obj->m_sTaskResult = task_result;
	}
	// notify waiting tasks
	obj->m_cTaskCv.notify_all();
}

void LaserTrackerTesterDlg::DynamicMeasurementCallback(
    const api::lt::MeasurementResult& result, void* callback_parameter) 
{
	if (!callback_parameter) 
	{
		return;
	}
	// aquire task mutex and set task result to task status
	auto obj = static_cast<LaserTrackerTesterDlg*>(callback_parameter);
	if (result.error_code == api::lt::ErrorType::Success) 
	{
		obj->m_ofsMeasurementFile << std::setw(12) << std::right;
		obj->m_ofsMeasurementFile << result.measured_point.x << "\t" << result.measured_point.y << "\t" << result.measured_point.z << "\n";
    } 
	else if (result.error_code == api::lt::ErrorType::MultiSmrMeasurementFinished) 
	{
		obj->m_ofsMeasurementFile << "Data collection finished\n";
		obj->m_ofsMeasurementFile.close();
	}
	else 
	{
		obj->m_ofsMeasurementFile << "Error occured: " << (int)result.error_code << "\n";
	}
}
void LaserTrackerTesterDlg::TTLMeasurementCallback(const api::lt::MeasurementResultTTL& result,
                                                   void* callback_parameter) 
{
	if (!callback_parameter)
	{
		return;
	}
    auto obj = static_cast<LaserTrackerTesterDlg*>(callback_parameter);
    if (result.error_code == api::lt::ErrorType::Success) 
	{
        obj->m_ofsMeasurementFile << std::setw(12) << std::right;
		obj->m_ofsMeasurementFile << result.timestamp << "\t" << result.measured_point.x << "\t"
                                << result.measured_point.y
                                << "\t" << result.measured_point.z << "\n";
    }
}

void LaserTrackerTesterDlg::MeasurementProbeCallback(const api::lt::MeasurementResultProbe &result, void* callback_parameter)
{
	if (!callback_parameter)
	{
		return;
	}
	auto obj = static_cast<LaserTrackerTesterDlg*>(callback_parameter);
    if (result.error_code == api::lt::ErrorType::Success) 
	{
        obj->m_ofsMeasurementFile << std::setw(12) << std::right;
		obj->m_ofsMeasurementFile << result.tip_position.x << "\t" << result.tip_position.y << "\t"
                                << result.tip_position.z
                                << "\t" << result.direction_vector.x << "\t" << result.direction_vector.y << '\t'
								<< result.direction_vector.z << '\n';
    }
}
 

void LaserTrackerTesterDlg::OnBnClickedButtonConnect() 
{
	BYTE n0, n1, n2, n3;
	((CIPAddressCtrl *)GetDlgItem(IDC_IPADDRESS))->GetAddress(n0, n1, n2, n3);
	auto ip = std::to_string(n0) + ".";
	ip += std::to_string(n1) + ".";
	ip += std::to_string(n2) + ".";
	ip += std::to_string(n3);
	if (!m_Device) 
	{
  		m_Device = new api::lt::LaserTrackerInterface();
    }
	m_sTaskResult = api::lt::TaskResult();

	auto ret = m_Device->Connect(api::lt::DeviceModel::RadianPro, ip.c_str(), nullptr, &LaserTrackerTesterDlg::TaskCallbackFunction, this);
	if (ret != api::lt::ErrorType::Success)
	{
		MessageBox(_T("Connection failed"), _T("Tester"), MB_OK);
		return;
	}
	// wait for connection to finish
	ret = (api::lt::ErrorType)WaitForCallbackResult(api::lt::TaskMode::Connect);
	if (ret != api::lt::ErrorType::Success)
	{
		if (ret != api::lt::ErrorType::AccessoryPrmFileNotFound) 
		{
			char errormessage[api::lt::MAX_ERROR_STRING_LENGTH];
			m_Device->GetErrorMessage(ret, errormessage);
			MessageBoxA(this->GetSafeHwnd(), errormessage, "API Error message", MB_OK | MB_ICONERROR);
			delete m_Device;
			m_Device = nullptr;
			return;
		}
		else
		{
			// since the PRM is managed on the device for the iLT, this is required for the first time users
			if (!HandleAccessoryPRMFile(ip))
			{
				return;
			}
		}
	}
	DWORD id;
	CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)LaserTrackerTesterDlg::WorkerProc, this, 0, &id);
	SetTimer(0x01, 100, NULL);
	SetTimer(0x2, 2000, NULL);
	CString str0;
	str0.Format(_T("%s: Connected"), GetCurrentDateTime().c_str());
	GetDlgItem(IDC_EDIT_STATUS)->SetWindowText(str0);
	auto device_info = m_Device->GetDeviceInformation();
	std::string sn(device_info.serial_number);
	std::wstring w_serial_number(sn.begin(), sn.end());
	std::string fw_ver(device_info.device_firmware_version);
	std::wstring w_version_number(fw_ver.begin(), fw_ver.end());
	m_AccModel = device_info.accessory_model;
	std::wstring acc(L"None");
	switch (device_info.accessory_model) 
	{ 
		case api::lt::AccessoryModel::STS:
			acc = std::wstring(L"STS");
			break;

		case api::lt::AccessoryModel::vProbe2:
			acc = std::wstring(L"vProbe");
			break;

		case api::lt::AccessoryModel::WSTS:
			acc = std::wstring(L"Wireless STS");
			break;

		default:
			break;
	}
	CString str;
	str.Format(_T("Serial Number: %s \r\nVersion Number: %s\r\nAccessory Model: %s\r\n"), w_serial_number.c_str(), w_version_number.c_str(), acc.c_str());
	GetDlgItem(IDC_SYSTEMINFO)->SetWindowText(str);
	// set up video stream window
	//if (m_dlgVideoStream)
	//{
	//	delete m_dlgVideoStream;
	//}
	//m_dlgVideoStream = new VideoStreamDlg(m_Device, this);
}


void LaserTrackerTesterDlg::OnBnClickedButtonDisconnect()
{
	KillTimer(0x1);
	KillTimer(0x2);
	// aquire task mutex and set task result to task status
	{
		std::lock_guard<std::mutex> lock(m_mTaskMtx);
		m_sTaskResult.task = api::lt::TaskMode::Idle;
	}

	//if (m_dlgVideoStream)
	//{
	//	if (m_dlgVideoStream->IsWindowOpen())
	//	{
	//		m_dlgVideoStream->OnClose();
	//	}
	//	delete m_dlgVideoStream;
	//	m_dlgVideoStream = nullptr;
	//}
	// notify waiting tasks
	m_cTaskCv.notify_all();
	if (m_Device) 
	{ 
		m_Device->Disconnect();	   
		delete m_Device;
		m_Device = nullptr;
    }
	CString str;
	str.Format(_T("%s: Disconnected"), GetCurrentDateTime().c_str());
	GetDlgItem(IDC_EDIT_STATUS)->SetWindowText(str);
}

void LaserTrackerTesterDlg::VideoStreamClosedEvent()
{
	//delete m_dlgVideoStream;
	//m_dlgVideoStream = nullptr;
}


void LaserTrackerTesterDlg::OnBnClickedButtonVirtuallevel()
{
	std::ifstream vl("VirtualLevel.txt");
	if (vl.is_open()) 
	{
        if (MessageBox(_T("Persistence level has been identified, Do you want to apply it?"), _T("API Message"), MB_YESNO | MB_ICONQUESTION) == IDYES) 
		{
			float matrix[9];
			for (int i = 0; i < 9; ++i) 
			{
				vl >> matrix[i];
			}
			auto ret = m_Device->SetVirtualLevelMatrix(matrix);
			if (ret == api::lt::ErrorType::Success) 
			{
				vl.close();
				return;
			}
        }
		vl.close();
	}
	auto ret = m_Device->VirtualLevel();
	if (ret != api::lt::ErrorType::Success)
	{
		MessageBox(_T("VirtualLevel failed"), _T("Tester"), MB_OK);
		return;
	}
}


void LaserTrackerTesterDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: Add your message handler code here and/or call default
	switch (nIDEvent) 
	{
		case 0x1:
		{
			api::lt::RealTimeInfo rt_info;
			auto ret = m_Device->GetRealTimeData(rt_info);
			if (ret == api::lt::ErrorType::Success) 
			{
				CString str, str0;
				str0.Format(_T("Timestamp: \t%d\r\n"), rt_info.timestamp);
				str += str0;
				str0.Format(_T("Az: \t%12.3f\r\n"), rt_info.az);
				str += str0;
				str0.Format(_T("El: \t%12.3f\r\n"), rt_info.el);
				str += str0;
				str0.Format(_T("Distance: \t%12.3f\r\n"), rt_info.distance);
				str += str0;
				str0.Format(_T("X: \t%12.3f\r\n"), rt_info.x);
				str += str0;
				str0.Format(_T("Y: \t%12.3f\r\n"), rt_info.y);
				str += str0;
				str0.Format(_T("Z: \t%12.3f\r\n"), rt_info.z);
				str += str0;
				if (rt_info.is_warmed_up) 
				{
					str0.Format(_T("Warm up status: \tDone\r\n"));
				} 
				else 
				{
					str0.Format(_T("Warm up status: \tWarming up\r\n"));
				}
				str += str0;
				if (rt_info.is_tracking) 
				{
					str0.Format(_T("Tracking status: \tTracking\r\n"));
                }
				else
				{
					str0.Format(_T("Tracking status: \tNot Tracking\r\n"));
                }
				str += str0;
				if (rt_info.is_measurement_valid) 
				{     
					str0.Format(_T("Measurement valid status: \tValid\r\n"));
                }
				else 
				{
					str0.Format(_T("Measurement valid status: \tNot Valid\r\n"));
                }
				str += str0;
				str0.Format(_T("Battery Status: \t%d\r\n"), rt_info.battery_status);
				str += str0;
				if (m_AccModel == api::lt::AccessoryModel::STS) 
				{
					str0.Format(_T("Pitch: \t%12.3f\r\n"), rt_info.sts_info.pitch);
					str += str0;
					str0.Format(_T("Yaw: \t%12.3f\r\n"), rt_info.sts_info.yaw);
					str += str0;
					str0.Format(_T("Roll: \t%12.3f\r\n"), rt_info.sts_info.roll);
					str += str0;
					str0.Format(_T("Az: \t%12.3f\r\n"), rt_info.sts_info.sts_az);
					str += str0;
					str0.Format(_T("El: \t%12.3f\r\n"), rt_info.sts_info.sts_el);
					str += str0;
					if (rt_info.sts_info.is_sts_tracking) 
					{
						str0.Format(_T("STS Tracking status: Tracking\r\n"));
					}
					else
					{
						str0.Format(_T("STS Tracking status: Not Tracking\r\n"));
					}
					str += str0;
				}
				else if (m_AccModel == api::lt::AccessoryModel::vProbe2) 
				{
					str0.Format(_T("Pitch: \t%12.3f\r\n"), rt_info.vp2_info.pitch);
					str += str0;
					str0.Format(_T("Yaw: \t%12.3f\r\n"), rt_info.vp2_info.yaw);
					str += str0;
					str0.Format(_T("Roll: \t%12.3f\r\n"), rt_info.vp2_info.roll);
					str += str0;
					if (rt_info.vp2_info.is_locked_on) 
					{
						str0.Format(_T("vProbe Locked status: Locked\r\n"));
					}
					else
					{
						str0.Format(_T("vProbe Locked status: Not Locked\r\n"));
					}
					str += str0;
				}

				GetDlgItem(IDC_EDIT_RTDATA)->SetWindowText(str);
            }		
			break;
		}

		case 0x2:
		{
			api::lt::WeatherStationInfo info;
			if (m_Device) 
			{
				m_Device->GetWeatherStationData(info);
				CString str, str0;
				str0.Format(_T("Air Temperature\t:%f\r\n"), info.air_temperature);
				str += str0;
				str0.Format(_T("Air Pressure\t:%f\r\n"), info.air_pressure);
				str += str0;
				str0.Format(_T("Air Humidity\t:%f\r\n"), info.air_humidity);
				str += str0;
				GetDlgItem(IDC_EDIT_WEATHER_STATION)->SetWindowText(str);
			}
		}
	}

	CDialogEx::OnTimer(nIDEvent);
}


void LaserTrackerTesterDlg::OnBnClickedButtonHome()
{
	int radio_button = GetCheckedRadioButton(IDC_RADIO_ONEHALFINCH, IDC_RADIO_SEVENEIGHTHINCH);
	int smr_index = 0;
	switch (radio_button) 
	{ 
		case IDC_RADIO_ONEHALFINCH: 
			smr_index = 0; 
			break;

		case IDC_RADIO_HALF_INCHSMR:
			smr_index = 1;
			break;

		case IDC_RADIO_SEVENEIGHTHINCH:
			smr_index = 2;
			break;
	}
	auto ret = m_Device->Home((api::lt::SmrSize)smr_index);
	if (ret != api::lt::ErrorType::Success)
	{
		MessageBox(_T("Home failed"), _T("Tester"), MB_OK);
		return;
	}
}


void LaserTrackerTesterDlg::OnBnClickedButtonPointtocartesian()
{
	UpdateData();
	api::lt::Vector3<float> point{m_fX, m_fY, m_fZ};
	auto ret = m_Device->PointToCartesian(point);
	if (ret != api::lt::ErrorType::Success)
	{
		MessageBox(_T("PointTo failed"), _T("Tester"), MB_OK);
		return;
	}
}


void LaserTrackerTesterDlg::OnBnClickedButtonPointtoSpherical()
{
	UpdateData();
	api::lt::PolarVector3fd point{m_fX, m_fY, m_fZ};
	auto ret = m_Device->PointToSpherical(point);
	if (ret != api::lt::ErrorType::Success)
	{
		MessageBox(_T("PointTo failed"), _T("Tester"), MB_OK);
		return;
	}
}


void LaserTrackerTesterDlg::OnBnClickedCheckEnablecameraSearch()
{
	if (((CButton *)GetDlgItem(IDC_CHECK_ENABLECAMERA_SEARCH))->GetCheck() == BST_CHECKED) 
	{
		auto ret = m_Device->EnableCameraSearch(true);
		if (ret != api::lt::ErrorType::Success)
		{
			MessageBox(_T("EnableCameraSearch failed"), _T("Tester"), MB_OK);
			return;
		}
		CString str;
		str.Format(_T("%s: EnableCameraSearch enabled"), GetCurrentDateTime().c_str());
		GetDlgItem(IDC_EDIT_STATUS)->SetWindowText(str);
	}
	else
	{
		auto ret = m_Device->EnableCameraSearch(false);
		if (ret != api::lt::ErrorType::Success)
		{
			MessageBox(_T("EnableCameraSearch failed"), _T("Tester"), MB_OK);
			return;
		}
		CString str;
		str.Format(_T("%s: EnableCameraSearch disabled"), GetCurrentDateTime().c_str());
		GetDlgItem(IDC_EDIT_STATUS)->SetWindowText(str);
	}
}

void LaserTrackerTesterDlg::OnBnClickedAbortProcedure()
{
	auto ret = m_Device->AbortProcedure();
	GetDlgItem(IDC_EDIT_STATUS)->SetWindowText(_T("AbortProcedure successful"));
}


void LaserTrackerTesterDlg::OnBnClickedButtonStaticDatacollection()
{
	std::ofstream fout("StaticDataPoints.txt", std::ios::app);
	UpdateData();
	api::lt::Vector3<float> point;
	float rms_error;
	auto ret = m_Device->GetSinglePointMeasurement((uint32_t)m_uiAverageTime, point, rms_error);
	if (ret != api::lt::ErrorType::Success) 
	{
		MessageBox(_T("CollectSinglePointMeasurement failed"), _T("Tester"), MB_OK);
		return;
	}
	CString str;
	str.Format(_T("%s: CollectSinglePointMeasurement finished\r\nx: %f, y: %f, z:%f\r\n"), GetCurrentDateTime().c_str(), point.x, point.y, point.z);
	fout << std::fixed << std::setprecision(4);
	fout << std::setw(12) << std::left;
	fout << point.x << "\t";
	fout << std::setw(12) << std::left;
	fout << point.y << "\t";
	fout << std::setw(12) << std::left;
	fout << point.z << "\n";
	GetDlgItem(IDC_EDIT_STATUS)->SetWindowText(str);
	fout.close();
}


void LaserTrackerTesterDlg::OnClose()
{
	KillTimer(0x1);	
	if (m_Device) 
	{
		auto ret = m_Device->Disconnect();
		if (ret != api::lt::ErrorType::Success)
		{
			MessageBox(_T("Disconnection failed"), _T("Tester"), MB_OK);
			return;
		}  
		delete m_Device;
		m_Device = nullptr;
	}
	GetDlgItem(IDC_EDIT_STATUS)->SetWindowText(_T("Disconnected"));
	CDialogEx::OnClose();
}


void LaserTrackerTesterDlg::OnBnClickedRelaxmotors()
{
	if (m_Device)
	{
		auto ret = m_Device->SwitchToIdle();
		if (ret != api::lt::ErrorType::Success)
		{
			MessageBox(_T("RelaxMotors failed"), _T("Tester"), MB_OK);
			return;
		}
		GetDlgItem(IDC_EDIT_STATUS)->SetWindowText(_T("Motors relaxed"));
	}
}


void LaserTrackerTesterDlg::OnBnClickedTrack()
{
	if (m_Device)
	{
		auto ret = m_Device->SwitchToTrack();
		if (ret != api::lt::ErrorType::Success)
		{
			MessageBox(_T("Track failed"), _T("Tester"), MB_OK);
			return;
		}
		GetDlgItem(IDC_EDIT_STATUS)->SetWindowText(_T("Track finished"));
	}
}

std::wstring LaserTrackerTesterDlg::GetCurrentDateTime()
{
	auto end = std::chrono::system_clock::now();
	std::time_t end_time = std::chrono::system_clock::to_time_t(end);
	auto str = std::string(std::ctime(&end_time));
	return std::wstring(str.begin(), str.end());
}

int LaserTrackerTesterDlg::WaitForCallbackResult(const api::lt::TaskMode& task) 
{
	// acquire mutex and wait for notification that matches waiting task
	std::unique_lock<std::mutex> lock(m_mTaskMtx);
	m_cTaskCv.wait(lock, [&] { return m_sTaskResult.task == task; });
	return static_cast<int>(m_sTaskResult.error_code);
}

int LaserTrackerTesterDlg::WaitForCallbackResult(const api::lt::TaskMode& task, int timeout_ms) 
{
	// acquire mutex and wait for notification that matches waiting task
	std::unique_lock<std::mutex> lock(m_mTaskMtx);
	if (!m_cTaskCv.wait_for(lock, std::chrono::milliseconds(timeout_ms), [&] { return m_sTaskResult.task == task; })) 
	{
		return static_cast<int>(api::lt::ErrorType::TaskOperationTimeout);
	}
	return static_cast<int>(m_sTaskResult.error_code);
}

void LaserTrackerTesterDlg::OnBnClickedButtonOpenivisionWindow() 
{
	auto ret = m_Device->OpenIvisionControlWindow();
	if (ret != api::lt::ErrorType::Success)
	{
		MessageBox(_T("OpenIvisionControlWindow failed"), _T("Tester"), MB_OK);
		return;
	}
}

void LaserTrackerTesterDlg::OnBnClickedButtonDynamictemporal() 
{
	UpdateData();
	m_ofsMeasurementFile.open("MeasurementFile.txt");
	m_ofsMeasurementFile << std::fixed << std::setprecision(3);
	m_ofsMeasurementFile << std::setw(12) << std::left;
	m_ofsMeasurementFile << "X (mm)\tY (mm)\tZ (mm)\n";
	auto ret = m_Device->StartTemporalDynamicMeasurement(m_iTimePeriod, &LaserTrackerTesterDlg::DynamicMeasurementCallback, this);
	if (ret != api::lt::ErrorType::Success) 
	{
		MessageBox(_T("StartTemporalDynamicMeasurement failed"), _T("Tester"), MB_OK);
        m_ofsMeasurementFile.close();
		return;
	}
}

void LaserTrackerTesterDlg::OnBnClickedButtonStopmeasurement() 
{
	m_Device->StopMeasurement();
	if (m_ofsMeasurementFile.is_open()) 
	{
		m_ofsMeasurementFile.close();
    }
}

void LaserTrackerTesterDlg::OnBnClickedButtonSpatialdynamic() 
{
	UpdateData();
	m_ofsMeasurementFile.open("MeasurementFile.txt");
	m_ofsMeasurementFile << std::fixed << std::setprecision(3);
	m_ofsMeasurementFile << std::setw(12) << std::left;
	m_ofsMeasurementFile << "X (mm)\tY (mm)\tZ (mm)\n";
	auto ret = m_Device->StartSpatialDynamicMeasurement(m_fDistance, &LaserTrackerTesterDlg::DynamicMeasurementCallback, this);
	if (ret != api::lt::ErrorType::Success) 
	{
		MessageBox(_T("StartTemporalDynamicMeasurement failed"), _T("Tester"), MB_OK);
        m_ofsMeasurementFile.close();
		return;
	}
}

void LaserTrackerTesterDlg::OnBnClickedButtonMultismrDatacollection() 
{
	UpdateData();
	m_ofsMeasurementFile.open("MultiSMR_MeasurementFile.txt");
	m_ofsMeasurementFile << std::fixed << std::setprecision(3);
	m_ofsMeasurementFile << std::setw(12) << std::left;
	m_ofsMeasurementFile << "X (mm)\tY (mm)\tZ (mm)\n";
	auto ret = m_Device->StartMultiSmrMeasurement(m_uiAverageTimeMultiSMR, &LaserTrackerTesterDlg::DynamicMeasurementCallback, this);
	if (ret != api::lt::ErrorType::Success) 
	{
		MessageBox(_T("StartMultiSmrMeasurement failed"), _T("Tester"), MB_OK);
        m_ofsMeasurementFile.close();
		return;
	}
}

void LaserTrackerTesterDlg::OnBnClickedButtonSpiralsearch() 
{
	UpdateData();
	auto ret = m_Device->SpiralSearch(m_fEstimatedDistance, m_fEstimatedRadius, m_uiTimeout);
	if (ret != api::lt::ErrorType::Success) 
	{
		MessageBox(_T("SpiralSearch failed"), _T("Tester"), MB_OK);
		return;
	}
}

void LaserTrackerTesterDlg::OnBnClickedButtonPointtocartesiansearch() 
{
	UpdateData();
	// disable the camera
	m_Device->EnableCameraSearch(false);
	// PointTo to the location
	api::lt::Vector3<float> point{m_fX, m_fY, m_fZ};
	auto ret = m_Device->PointToCartesian(point);
	if (ret != api::lt::ErrorType::Success)
	{
		MessageBox(_T("PointTo failed"), _T("Tester"), MB_OK);
		return;
	}
	// Wait for callback
	ret = (api::lt::ErrorType)WaitForCallbackResult(api::lt::TaskMode::PointToCartesian, 5000);
	if (ret != api::lt::ErrorType::Success)
	{
		char errormessage[api::lt::MAX_ERROR_STRING_LENGTH];
		m_Device->GetErrorMessage(ret, errormessage);
		MessageBoxA(this->GetSafeHwnd(), errormessage, "API Error message", MB_OK | MB_ICONERROR);
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(200));
	// enable the camera
	ret = m_Device->EnableCameraSearch(true);
	if (ret != api::lt::ErrorType::Success) 
	{
		char errormessage[api::lt::MAX_ERROR_STRING_LENGTH];
		m_Device->GetErrorMessage(ret, errormessage);
		MessageBoxA(this->GetSafeHwnd(), errormessage, "API Error message", MB_OK | MB_ICONERROR);
	}
}

void LaserTrackerTesterDlg::OnBnClickedButtonTtlmeasurement() 
{
	if (!m_bTTLChecked)
	{
		m_ofsMeasurementFile.open("MeasurementFileTTL.txt");
		m_ofsMeasurementFile << std::fixed << std::setprecision(3);
		m_ofsMeasurementFile << std::setw(12) << std::left;
		m_ofsMeasurementFile << "Timestamp\tX (mm)\tY (mm)\tZ (mm)\n";
		auto ret = m_Device->StartTTLMeasurement(&LaserTrackerTesterDlg::TTLMeasurementCallback, this);
		if (ret != api::lt::ErrorType::Success)
		{
            char errormessage[api::lt::MAX_ERROR_STRING_LENGTH];
            m_Device->GetErrorMessage(ret, errormessage);
            MessageBoxA(this->GetSafeHwnd(), errormessage, "API Error message", MB_OK | MB_ICONERROR);
            m_ofsMeasurementFile.close();
            return;
		}
        GetDlgItem(IDC_BUTTON_TTLMEASUREMENT)->SetWindowTextW(L"Stop TTL Measurement");
        m_bTTLChecked = true;
	} 
	else
	{
		auto ret = m_Device->StopTTLMeasurement();
		if (ret != api::lt::ErrorType::Success)
		{
            char errormessage[api::lt::MAX_ERROR_STRING_LENGTH];
            m_Device->GetErrorMessage(ret, errormessage);
            MessageBoxA(this->GetSafeHwnd(), errormessage, "API Error message", MB_OK | MB_ICONERROR);
            return;
		}
        GetDlgItem(IDC_BUTTON_TTLMEASUREMENT)->SetWindowTextW(L"Start TTL Measurement");
        m_bTTLChecked = false;
		m_ofsMeasurementFile.close();
	}
}


void LaserTrackerTesterDlg::OnBnClickedButtonSinglepointprobedatacollection()
{
	// Hardcoded average time to 750ms. It is adjustable
	auto ret = m_Device->StartSinglePointProbeMeasurement(750, &LaserTrackerTesterDlg::MeasurementProbeCallback, this);
	if (ret != api::lt::ErrorType::Success)
	{
        char errormessage[api::lt::MAX_ERROR_STRING_LENGTH];
        m_Device->GetErrorMessage(ret, errormessage);
        MessageBoxA(this->GetSafeHwnd(), errormessage, "API Error message", MB_OK | MB_ICONERROR);
        return;
	}
	m_ofsMeasurementFile.open("MeasurementFileStaticProbe.txt");
	m_ofsMeasurementFile << std::fixed << std::setprecision(3);
	m_ofsMeasurementFile << std::setw(12) << std::left;
	m_ofsMeasurementFile << "X (mm)\tY (mm)\tZ (mm)\tDirection X\tDirection Y\tDirection Z\n";
}


void LaserTrackerTesterDlg::OnBnClickedButtonDynamicprobedatacollection()
{
	// Hardcoded spatial distance to 0.2mm. It is adjustable
	auto ret = m_Device->StartSpatialDynamicProbeMeasurement(0.2f, &LaserTrackerTesterDlg::MeasurementProbeCallback, this);
	if (ret != api::lt::ErrorType::Success)
	{
        char errormessage[api::lt::MAX_ERROR_STRING_LENGTH];
        m_Device->GetErrorMessage(ret, errormessage);
        MessageBoxA(this->GetSafeHwnd(), errormessage, "API Error message", MB_OK | MB_ICONERROR);
        return;
	}
	m_ofsMeasurementFile.open("MeasurementFileDynamicProbe.txt");
	m_ofsMeasurementFile << std::fixed << std::setprecision(3);
	m_ofsMeasurementFile << std::setw(12) << std::left;
	m_ofsMeasurementFile << "X (mm)\tY (mm)\tZ (mm)\tDirection X\tDirection Y\tDirection Z\n";
}


void LaserTrackerTesterDlg::OnBnClickedButtonStopprobemeasurement()
{
	m_ofsMeasurementFile.close();
	m_Device->StopMeasurement();
}


void LaserTrackerTesterDlg::OnBnClickedButtonOpenprobeifcdialog()
{
	auto ret = m_Device->OpenProbeCalibrationWindow();
	if (ret != api::lt::ErrorType::Success)
	{
        char errormessage[api::lt::MAX_ERROR_STRING_LENGTH];
        m_Device->GetErrorMessage(ret, errormessage);
        MessageBoxA(this->GetSafeHwnd(), errormessage, "API Error message", MB_OK | MB_ICONERROR);
        return;
	}
}

bool LaserTrackerTesterDlg::HandleAccessoryPRMFile(const std::string &ip)
{
	// check with user if they want to upload the PRM file
	auto ret = this->MessageBox(L"Accessory PRM File is not found. Do you want to import it now?", L"API Laser Tracker Message", MB_YESNO | MB_ICONQUESTION);
	if (ret == IDYES)
	{
		// If yes, browse the file from explorer, get the path and call SDK function to set
		const TCHAR szFilter[] = _T("PRM Files (*.prm)|*.prm|");
		CFileDialog dlg(TRUE, _T("prm"), NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, szFilter, this);    
		if(dlg.DoModal() == IDOK)
		{
			CString sFilePath = dlg.GetPathName();
			CStringA s2( sFilePath );
			auto ret_status = m_Device->SetAccessoryPrmFile(s2);
			if (ret_status != api::lt::ErrorType::Success) 
			{
				char error_message[255];
				m_Device->GetErrorMessage(ret_status, error_message);
				MessageBoxA(this->GetSafeHwnd(), error_message, "API Error message", MB_OK | MB_ICONERROR);
				return false;
			}
			// this is to make sure there is no memory leak, disconnecting and connecting again
			m_Device->Disconnect();
			Sleep(100);
			m_sTaskResult = api::lt::TaskResult();
			ret_status = m_Device->Connect(api::lt::DeviceModel::iLT, ip.c_str(), nullptr, &LaserTrackerTesterDlg::TaskCallbackFunction, this);
			if (ret_status != api::lt::ErrorType::Success) 
			{
			  char error_message[255];
			  m_Device->GetErrorMessage(ret_status, error_message);
			  MessageBoxA(this->GetSafeHwnd(), error_message, "API Error message", MB_OK | MB_ICONERROR);
			  return false;
			}
			// wait for connection to finish
			ret_status = (api::lt::ErrorType)WaitForCallbackResult(api::lt::TaskMode::Connect);
			if (ret_status != api::lt::ErrorType::Success)
			{
				char error_message[255];
				m_Device->GetErrorMessage(ret_status, error_message);
				MessageBoxA(this->GetSafeHwnd(), error_message, "API Error message", MB_OK | MB_ICONERROR);
				return false;
			}
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		// If no, return the error message saying accessory PRM file not found
		char errormessage[api::lt::MAX_ERROR_STRING_LENGTH];
        m_Device->GetErrorMessage(api::lt::ErrorType::AccessoryPrmFileNotFound, errormessage);
        MessageBoxA(this->GetSafeHwnd(), errormessage, "API Error message", MB_OK | MB_ICONERROR);
		return false;
	}
}


void LaserTrackerTesterDlg::OnBnClickedButtonOpenvideostreaming()
{
	//if (m_Device && m_Device->IsDeviceConnected() && m_dlgVideoStream && !m_dlgVideoStream->IsWindowOpen())
	//{
	//	m_dlgVideoStream->Create(IDD_VIDEOSTREAM_DLG, this);
	//	m_dlgVideoStream->ShowWindow(SW_SHOW);
	//}
}
