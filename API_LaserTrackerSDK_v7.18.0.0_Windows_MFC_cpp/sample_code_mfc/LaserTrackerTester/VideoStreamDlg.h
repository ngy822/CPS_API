#pragma once
#include "api/LaserTrackerInterface.h"

// VideoStreamDlg dialog

class VideoStreamDlg : public CDialogEx
{
	DECLARE_DYNAMIC(VideoStreamDlg)

	bool m_bIsWindowOpen;
	api::lt::LaserTrackerInterface *m_device;
	unsigned int m_uiWidth;
	unsigned int m_uiHeight;

public:
	VideoStreamDlg(api::lt::LaserTrackerInterface* device, CWnd* pParent = nullptr);   // standard constructor
	virtual ~VideoStreamDlg();
	virtual BOOL OnInitDialog();
	bool IsWindowOpen();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_VIDEOSTREAM_DLG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	void Stream();
	bool DisplayBitmap(unsigned char* pbuf, uint32_t width, uint32_t height);

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnClose();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
};
