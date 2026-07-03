// VideoStreamDlg.cpp : implementation file
//

#include "pch.h"
#include "LaserTrackerTesterDlg.h"
#include "VideoStreamDlg.h"
#include "afxdialogex.h"
#include "resource.h"
#include <string>

#define VIDEO_STREAM 0x101

// VideoStreamDlg dialog

IMPLEMENT_DYNAMIC(VideoStreamDlg, CDialogEx)

VideoStreamDlg::VideoStreamDlg(api::lt::LaserTrackerInterface* device, CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_VIDEOSTREAM_DLG, pParent)
{
	m_device = device;
	m_bIsWindowOpen = false;
}

VideoStreamDlg::~VideoStreamDlg()
{
}

BOOL VideoStreamDlg::OnInitDialog() 
{
	CDialogEx::OnInitDialog();
	m_bIsWindowOpen = true;
	m_device->GetVideoFrameImage(nullptr, m_uiWidth, m_uiHeight);
    SetTimer(VIDEO_STREAM, 100, NULL);
	return TRUE;
}

bool VideoStreamDlg::IsWindowOpen() 
{
	return m_bIsWindowOpen;
}

void VideoStreamDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


void VideoStreamDlg::Stream()
{
	uint8_t *video_frame = new uint8_t[m_uiWidth * m_uiHeight * 3];
	auto ret = m_device->GetVideoFrameImage(video_frame, m_uiWidth, m_uiHeight);
	if (ret == api::lt::ErrorType::Success)
	{
		DisplayBitmap(video_frame, m_uiWidth, m_uiHeight);
	}
	delete[] video_frame;
}

bool VideoStreamDlg::DisplayBitmap(unsigned char* pbuf, uint32_t width, uint32_t height)
{	
	HWND video = ::GetDlgItem(this->GetSafeHwnd(), IDC_VIDEO_STREAM_CONTROL);
	CWnd *cwnd_video = CWnd::FromHandle(video);
    RECT video_rect;
	cwnd_video->GetWindowRect(&video_rect);
    HDC hdc_video, hdcsrc_video;
	int nWidth = video_rect.right - video_rect.left;
    int nHeight = video_rect.bottom - video_rect.top;
	hdc_video = (cwnd_video->GetDC())->GetSafeHdc();
    hdcsrc_video = CreateCompatibleDC(hdc_video);

	BITMAPINFO bmi;
    BITMAPINFOHEADER bhr;
    memset(&bmi, 0, sizeof(bmi));
    bhr.biSize = sizeof(bhr);
    bhr.biWidth = (int)width;
    bhr.biHeight = -1 * (int)height;
    bhr.biPlanes = 1;
    bhr.biBitCount = 24;
    bhr.biCompression = BI_RGB;
    bhr.biSizeImage = width * height * 3;
    bmi.bmiHeader = bhr;
    bmi.bmiColors[0].rgbBlue = 255;
    bmi.bmiColors[0].rgbGreen = 255;
    bmi.bmiColors[0].rgbRed = 255;
    bmi.bmiColors[0].rgbReserved = 0;

    HBITMAP hbmold, hbmnew;
    hbmnew = CreateDIBitmap(hdc_video, &bhr, CBM_INIT, pbuf, &bmi, DIB_RGB_COLORS);
	if(hbmnew)
	{			
		hbmold = (HBITMAP)SelectObject(hdcsrc_video, hbmnew);
		SetStretchBltMode(hdc_video, COLORONCOLOR);
		bool bSuc = StretchBlt(hdc_video, 0, 0, nWidth, nHeight, hdcsrc_video, 0, 0, width, height, SRCCOPY);
		DeleteObject(hbmold);
	}	
	DeleteObject(hbmnew);
	DeleteDC(hdcsrc_video);
	DeleteDC(hdc_video);
	return true;
}

BEGIN_MESSAGE_MAP(VideoStreamDlg, CDialogEx)
	ON_WM_CLOSE()
	ON_WM_TIMER()
END_MESSAGE_MAP()


// VideoStreamDlg message handlers

void VideoStreamDlg::OnClose() 
{
	KillTimer(VIDEO_STREAM);
	m_bIsWindowOpen = false;
	CDialogEx::OnClose();
}

void VideoStreamDlg::OnTimer(UINT_PTR nIDEvent) 
{
	switch (nIDEvent)
	{
		case VIDEO_STREAM:
            Stream();
            break;
	}
	CDialogEx::OnTimer(nIDEvent);
}
