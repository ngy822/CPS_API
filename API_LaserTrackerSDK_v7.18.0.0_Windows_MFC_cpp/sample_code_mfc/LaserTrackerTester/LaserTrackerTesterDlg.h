
// LaserTrackerTesterDlg.h : header file
//

#pragma once
#include <memory>
#include <string>
#include <mutex>
#include <condition_variable>
#include <fstream>
//#include "VideoStreamDlg.h"

#include "api/LaserTrackerInterface.h"


// LaserTrackerTesterDlg dialog
class LaserTrackerTesterDlg : public CDialogEx
{
// Construction
public:
	LaserTrackerTesterDlg(CWnd* pParent = nullptr);	// standard constructor
	virtual ~LaserTrackerTesterDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_LASERTRACKERTESTER_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	static void WorkerProc(LaserTrackerTesterDlg *p);


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
    static void TaskCallbackFunction(const api::lt::TaskResult &task_result, void *callback_parameter);
	static void DynamicMeasurementCallback(const api::lt::MeasurementResult &result, void *callback_parameter);
    static void TTLMeasurementCallback(const api::lt::MeasurementResultTTL &result,
                                           void *callback_parameter);
	static void MeasurementProbeCallback(const api::lt::MeasurementResultProbe &result, void *callback_parameter);
	afx_msg void OnBnClickedButtonConnect();
	afx_msg void OnBnClickedButtonDisconnect();
	void VideoStreamClosedEvent();
	afx_msg void OnBnClickedButtonVirtuallevel();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnBnClickedButtonHome();
	afx_msg void OnBnClickedButtonPointtocartesian();
	afx_msg void OnBnClickedButtonPointtoSpherical();
	afx_msg void OnBnClickedCheckEnablecameraSearch();
	afx_msg void OnBnClickedAbortProcedure();
	afx_msg void OnBnClickedButtonStaticDatacollection();
	afx_msg void OnClose();
	afx_msg void OnBnClickedRelaxmotors();
	afx_msg void OnBnClickedTrack();
	afx_msg void OnBnClickedButtonOpenivisionWindow();
	afx_msg void OnBnClickedButtonDynamictemporal();
	afx_msg void OnBnClickedButtonStopmeasurement();
	afx_msg void OnBnClickedButtonSpatialdynamic();
	afx_msg void OnBnClickedButtonMultismrDatacollection();
	afx_msg void OnBnClickedButtonSpiralsearch();
	afx_msg void OnBnClickedButtonPointtocartesiansearch();
	afx_msg void OnBnClickedButtonTtlmeasurement();
	afx_msg void OnBnClickedButtonSinglepointprobedatacollection();
	afx_msg void OnBnClickedButtonDynamicprobedatacollection();
	afx_msg void OnBnClickedButtonStopprobemeasurement();
	afx_msg void OnBnClickedButtonOpenprobeifcdialog();

private:
	bool HandleAccessoryPRMFile(const std::string &ip_address);

private:
	api::lt::LaserTrackerInterface *m_Device;
	float m_fX;
	float m_fY;
	float m_fZ;
	unsigned int m_uiAverageTime;
	unsigned int m_uiAverageTimeMultiSMR;
	unsigned int m_uiAvergeTimeStablePointSMR;
	int m_iPointToNotify;
	std::wstring GetCurrentDateTime();
    int WaitForCallbackResult(const api::lt::TaskMode &task);
    int WaitForCallbackResult(const api::lt::TaskMode &task, int timeout_ms);
	api::lt::TaskResult m_sTaskResult;
	std::mutex m_mTaskMtx;
	std::condition_variable m_cTaskCv;
	api::lt::TaskMode m_CurrentTask;
	int m_iTimePeriod;
	float m_fDistance;
	std::ofstream m_ofsMeasurementFile;   
	api::lt::AccessoryModel m_AccModel;
	float m_fAz;
	float m_fEl;
	float m_fEstimatedDistance;
	float m_fEstimatedRadius;
	unsigned int m_uiTimeout;
    bool m_bTTLChecked;
	//VideoStreamDlg *m_dlgVideoStream;

public:
	afx_msg void OnBnClickedButtonOpenvideostreaming();
};
