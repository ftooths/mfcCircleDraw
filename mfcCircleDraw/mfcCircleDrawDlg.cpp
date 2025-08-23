// mfcCircleDrawDlg.cpp : 구현 파일
//

#include "pch.h"
#include "framework.h"
#include "mfcCircleDraw.h"
#include "mfcCircleDrawDlg.h"
#include "afxdialogex.h"
#include <iostream>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// 응용 프로그램 정보에 사용되는 CAboutDlg 대화 상자입니다.

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

	// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	// 구현입니다.
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


// CmfcCircleDrawDlg 대화 상자



CmfcCircleDrawDlg::CmfcCircleDrawDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_MFCCIRCLEDRAW_DIALOG, pParent)
	, m_bDragging(false) // 생성자에서 멤버 변수 초기화. 드래그 상태는 false로 시작
	, m_nDragIndex(-1)   // 드래그 중인 점의 인덱스는 -1, 없음으로 초기화
	, m_bStopAnimation(false) // 애니메이션 스레드 중지 플래그 초기화
	, m_bAnimationRunning(false) // 애니메이션 실행 상태 플래그 초기화
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CmfcCircleDrawDlg::~CmfcCircleDrawDlg()
{
	// 소멸자 호출, 프로그램이 종료될 때 애니메이션 스레드가 실행 중이라면 안전하게 종료시켜 리소스 누수 방지
	StopAnimationThread();
}

void CmfcCircleDrawDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

// 윈도우 메시지와 핸들러 함수를 연결하는 메시지 맵
BEGIN_MESSAGE_MAP(CmfcCircleDrawDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT() // 화면을 다시 그려야 할 때 OnPaint 함수 호출
	ON_WM_QUERYDRAGICON()
	ON_WM_LBUTTONDOWN() // 마우스 왼쪽 버튼 클릭 시 OnLButtonDown 함수 호출
	ON_WM_MOUSEMOVE()   // 마우스 이동 시 OnMouseMove 함수 호출
	ON_WM_LBUTTONUP()   // 마우스 왼쪽 버튼 뗄 때 OnLButtonUp 함수 호출
	ON_EN_CHANGE(IDC_EDIT_RADIUS, &CmfcCircleDrawDlg::OnEnChangeEditRadius) // 반지름 에디트 컨트롤 값 변경 시
	ON_EN_CHANGE(IDC_THICKNESS, &CmfcCircleDrawDlg::OnEnChangeThickness)   // 두께 에디트 컨트롤 값 변경 시
	ON_BN_CLICKED(IDC_BTN_RESET, &CmfcCircleDrawDlg::OnBnClickedBtnReset) // 리셋 버튼 클릭 시
	ON_BN_CLICKED(IDC_BTN_RANDOM_MOVE, &CmfcCircleDrawDlg::OnBnClickedBtnRandomMove) // 랜덤 이동 버튼 클릭 시
	ON_MESSAGE(WM_UPDATE_DISPLAY, &CmfcCircleDrawDlg::OnUpdateDisplay) // 작업 스레드로부터 화면 갱신 요청 메시지 처리
END_MESSAGE_MAP()


// CmfcCircleDrawDlg 메시지 처리기

BOOL CmfcCircleDrawDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 시스템 메뉴에 "정보..." 메뉴 항목을 추가합니다.

	// IDM_ABOUTBOX는 시스템 명령 범위에 있어야 합니다.
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

	// 이 대화 상자의 아이콘을 설정합니다.  응용 프로그램의 주 창이 대화 상자가 아닐 경우에는
	//  프레임워크가 이 작업을 자동으로 수행합니다.
	SetIcon(m_hIcon, TRUE);			// 큰 아이콘을 설정합니다.
	SetIcon(m_hIcon, FALSE);		// 작은 아이콘을 설정합니다.

	// 드로잉 버퍼를 초기화, 디버깅 위한 콘솔 창을 할당
	InitializeImageBuffer();
	AllocConsole();
	FILE* pFile;
	freopen_s(&pFile, "CONOUT$", "w", stdout); // stdout 스트림을 콘솔로 재지정하여 std::cout 사용 가능

	return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}

void CmfcCircleDrawDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

// 대화 상자를 최소화하지만 아이콘을 그리려면 아래 코드가 필요합니다.
//  문서/뷰 모델을 사용하는 MFC 응용 프로그램의 경우에는 프레임워크가 자동으로
//  이 작업을 수행합니다.

void CmfcCircleDrawDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 그리기를 위한 디바이스 컨텍스트입니다.

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 클라이언트 사각형에서 아이콘을 가운데에 맞춥니다.
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 아이콘을 그립니다.
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CPaintDC dc(this);
		// 미리 그려둔 m_image를 화면에 한 번에 복사.
		// 더블 버퍼링 사용, 화면 깜빡임을 방지하고 부드러운 그리기를 가능하게 함.
		if (!m_image.IsNull())
		{
			m_image.Draw(dc.GetSafeHdc(), 0, 0);
		}
	}
}

// 사용자가 최소화된 창을 끄는 동안에 커서가 표시되도록 시스템에서
//이 함수를 호출합니다.
HCURSOR CmfcCircleDrawDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


// --- 마우스 및 UI 이벤트 핸들러 ---

void CmfcCircleDrawDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	// 점이 3개 모두 찍힌 상태에서는 드래그 시작 여부를 판단
	if (m_clickPoints.size() == 3)
	{
		int nearestIndex = GetNearestPointIndex(point);
		// 클릭한 위치가 기존 점과 가깝다면 드래그 시작
		if (nearestIndex != -1)
		{
			m_bDragging = true;
			m_nDragIndex = nearestIndex;
			SetCapture(); // 마우스 입력을 현재 창으로 고정하여 창 밖으로 나가도 이벤트 받도록 함
			return;
		}
		// 기존 점 근처가 아니면 아무 동작도 하지 않음
		return;
	}

	// 점이 3개 미만일 경우 새로운 점을 추가
	if (m_clickPoints.size() < 3)
	{
		m_clickPoints.push_back(point);
		std::cout << "Point " << m_clickPoints.size() << " added at: (" << point.x << ", " << point.y << ")" << std::endl;
		RedrawScene(); // 추가된 점 화면을 다시 그림
	}

	CDialogEx::OnLButtonDown(nFlags, point);
}

void CmfcCircleDrawDlg::OnMouseMove(UINT nFlags, CPoint point)
{
	// 드래그 중인 상태일 때만 로직 실행
	if (m_bDragging && m_nDragIndex != -1)
	{
		// 드래그 중인 점의 좌표를 현재 마우스 위치로 이동
		m_clickPoints[m_nDragIndex] = point;
		// 이동하는 점 실시간으로 다시 그려서 보여줌 
		RedrawScene();
	}
	CDialogEx::OnMouseMove(nFlags, point);
}

void CmfcCircleDrawDlg::OnLButtonUp(UINT nFlags, CPoint point)
{
	// 마우스 왼쪽 버튼을 떼면 드래그 상태를 종료
	if (m_bDragging)
	{
		m_bDragging = false;
		m_nDragIndex = -1;
		ReleaseCapture(); // 마우스 캡처 해제
	}
	CDialogEx::OnLButtonUp(nFlags, point);
}

void CmfcCircleDrawDlg::OnEnChangeEditRadius()
{
	// 반지름 값이 변경되면 화면을 다시 그림. 점이 있을 때 만 갱신
	if (!m_clickPoints.empty())
	{
		RedrawScene();
	}
}

void CmfcCircleDrawDlg::OnEnChangeThickness()
{
	// 두께 값이 변경되면 화면을 다시 그림
	// 외접원은 점이 3개일 때만 그려지므로, 3개일 때만 갱신
	if (m_clickPoints.size() == 3)
	{
		RedrawScene();
	}
}

void CmfcCircleDrawDlg::OnBnClickedBtnReset()
{
	StopAnimationThread(); // 실행 중일 수 있는 애니메이션 스레드 정지

	m_clickPoints.clear(); // 저장된 모든 클릭 지점 삭제
	InitializeImageBuffer(); // 이미지 버퍼를 깨끗하게 초기화

	// UI의 좌표 표시 텍스트를 초기 상태로 되돌림
	SetDlgItemText(IDC_STATIC_POINT1, _T("Point 1:"));
	SetDlgItemText(IDC_STATIC_POINT2, _T("Point 2:"));
	SetDlgItemText(IDC_STATIC_POINT3, _T("Point 3:"));
	SetDlgItemText(IDC_EDIT_RADIUS, _T(""));
	SetDlgItemText(IDC_THICKNESS, _T(""));

	std::cout << "Reset complete." << std::endl;
}

void CmfcCircleDrawDlg::OnBnClickedBtnRandomMove()
{
	// 랜덤 이동은 점 3개가 모두 있어야 가능
	if (m_clickPoints.size() != 3)
	{
		AfxMessageBox(_T("Please click 3 points first to create a circle."));
		return;
	}
	// 애니메이션을 수행하는 별도의 스레드를 시작
	StartAnimationThread();
}

LRESULT CmfcCircleDrawDlg::OnUpdateDisplay(WPARAM wParam, LPARAM lParam)
{
	// 이 메시지는 워킹 스레드에서 UI 스레드로 화면 갱신을 요청하기 위해 사용됨
	// 워킹 스레드는 직접 UI를 제어할 수 없으므로, PostMessage를 통해 해당 핸들러를 호출
	Invalidate(FALSE); // 화면을 무효화하여 OnPaint()가 호출되도록 함
	UpdateWindow();    // 즉시 OnPaint()를 호출하여 화면을 갱신
	return 0;
}

// --- 핵심 그리기 및 로직 함수 ---

void CmfcCircleDrawDlg::InitializeImageBuffer()
{
	// 기존 이미지가 있다면 파괴하여 메모리 누수 방지
	if (!m_image.IsNull())
		m_image.Destroy();

	// 8비트 그레이스케일 이미지 버퍼 생성
	int nWidth = 640;
	int nHeight = 480;
	int nBpp = 8;
	// 높이를 음수로 주어 Top-Down 비트맵을 생성. 좌표 계산이 직관적으로 변함.
	m_image.Create(nWidth, -nHeight, nBpp);

	// 그레이스케일 색상 팔레트 설정
	static RGBQUAD rgb[256];
	for (int i = 0; i < 256; i++)
		rgb[i].rgbRed = rgb[i].rgbGreen = rgb[i].rgbBlue = static_cast<BYTE>(i);
	m_image.SetColorTable(0, 256, rgb);

	// 버퍼의 모든 픽셀을 흰색(0xff)으로 초기화
	unsigned char* fm = (unsigned char*)m_image.GetBits();
	int nPitch = m_image.GetPitch();
	memset(fm, 0xff, nWidth * nHeight);

	Invalidate(FALSE); // 초기화된 버퍼를 화면에 표시하도록 갱신 요청
}

void CmfcCircleDrawDlg::RedrawScene()
{
	// 이미지 버퍼가 준비되지 않았다면 그리기 수행 안함
	if (m_image.IsNull()) return;

	// 1. 이미지 버퍼를 흰색으로 깨끗이 지움 
	unsigned char* fm = (unsigned char*)m_image.GetBits();
	int nPitch = m_image.GetPitch();
	memset(fm, 0xff, m_image.GetWidth() * m_image.GetHeight());

	// 2. UI 컨트롤에서 현재 설정된 radius와 두께 값 가져옴
	float currentRadius = GetCurrentRadius();
	float currentThickness = GetCurrentThickness();

	// 3. 사용자가 클릭한 각 지점에 대해 채워진 원을 그림
	for (const auto& point : m_clickPoints)
	{
		DrawFilledCircle(point.x, point.y, currentRadius);
	}

	// 4. 클릭 지점이 3개일 경우, 정원 계산하고 그림
	if (m_clickPoints.size() == 3)
	{
		double centerX, centerY, radius;
		// 외접원 계산이 성공했을 경우에만 원을 그림
		if (CalculateCircumcircle(m_clickPoints[0], m_clickPoints[1], m_clickPoints[2], centerX, centerY, radius))
		{
			DrawHollowCircle(centerX, centerY, radius, currentThickness);
		}
	}

	UpdateCoordinateDisplay();

	Invalidate(FALSE);
}

void CmfcCircleDrawDlg::DrawFilledCircle(int centerX, int centerY, float radius, unsigned char color)
{
	unsigned char* fm = (unsigned char*)m_image.GetBits();
	int nWidth = m_image.GetWidth();
	int nHeight = m_image.GetHeight();
	int nPitch = m_image.GetPitch();
	int r = static_cast<int>(radius);

	// 원을 포함하는 최소 사각형(Bounding Box) 영역만 순회하게 제약
	for (int y = -r; y <= r; y++)
	{
		for (int x = -r; x <= r; x++)
		{
			// 원 방정식을 만족하는 픽셀인지 검사.
			// 제곱근(sqrt) 연산을 피하기 위해 제곱 형태로 비교하여 효율을 높임.
			if (x * x + y * y <= r * r)
			{
				int pixelX = centerX + x;
				int pixelY = centerY + y;
				// 픽셀이 이미지 경계 내에 있는지 반드시 확인하여 메모리 침범 오류 방지
				if (pixelX >= 0 && pixelX < nWidth && pixelY >= 0 && pixelY < nHeight)
				{
					fm[pixelY * nPitch + pixelX] = color;
				}
			}
		}
	}
}

void CmfcCircleDrawDlg::DrawHollowCircle(double centerX, double centerY, double radius, float thickness, unsigned char color)
{
	unsigned char* fm = (unsigned char*)m_image.GetBits();
	int nWidth = m_image.GetWidth();
	int nHeight = m_image.GetHeight();
	int nPitch = m_image.GetPitch();

	// 원의 두께를 표현하기 위해 외부 원과 내부 원의 반지름을 계산.
	double outerRadiusSq = (radius + thickness / 2.0) * (radius + thickness / 2.0);
	double innerRadius = radius - thickness / 2.0;
	double innerRadiusSq = (innerRadius < 0) ? 0 : (innerRadius * innerRadius);

	// 불필요한 픽셀 검사를 줄이기 위해 Bounding Box 계산
	int r_out = static_cast<int>(radius + thickness / 2.0) + 1;
	int minX = max(0, static_cast<int>(centerX - r_out));
	int maxX = min(nWidth - 1, static_cast<int>(centerX + r_out));
	int minY = max(0, static_cast<int>(centerY - r_out));
	int maxY = min(nHeight - 1, static_cast<int>(centerY + r_out));

	// Bounding Box 내의 모든 픽셀을 순회
	for (int y = minY; y <= maxY; y++)
	{
		for (int x = minX; x <= maxX; x++)
		{
			double dx = x - centerX;
			double dy = y - centerY;
			double distSq = dx * dx + dy * dy;

			// 픽셀과 원 중심까지의 거리의 제곱이 내부 원과 외부 원 사이에 있을 때만 픽셀을 그림
			if (distSq >= innerRadiusSq && distSq <= outerRadiusSq)
			{
				fm[y * nPitch + x] = color;
			}
		}
	}
}

bool CmfcCircleDrawDlg::CalculateCircumcircle(CPoint p1, CPoint p2, CPoint p3, double& centerX, double& centerY, double& radius)
{
	// 세 점을 지나는 정원의 중심과 반지름을 계산하는 공식 구현
	double ax = p1.x, ay = p1.y;
	double bx = p2.x, by = p2.y;
	double cx = p3.x, cy = p3.y;

	double d = 2.0 * (ax * (by - cy) + bx * (cy - ay) + cx * (ay - by));

	// 분모 d가 0에 가까우면 세 점이 거의 일직선 상에 있다는 의미.
	// 이 경우 유일한 원을 정의할 수 없으므로 계산 실패를 반환.
	if (std::abs(d) < 1e-9)
	{
		std::cout << "Points are collinear; cannot form a unique circle." << std::endl;
		return false;
	}

	double aSq = ax * ax + ay * ay;
	double bSq = bx * bx + by * by;
	double cSq = cx * cx + cy * cy;

	// 외접원의 중심 좌표 계산
	centerX = (aSq * (by - cy) + bSq * (cy - ay) + cSq * (ay - by)) / d;
	centerY = (aSq * (cx - bx) + bSq * (ax - cx) + cSq * (bx - ax)) / d;

	// 반지름은 중심과 세 점 중 아무 점 사이의 거리로 계산
	double dx = centerX - ax;
	double dy = centerY - ay;
	radius = sqrt(dx * dx + dy * dy);

	return true;
}

// --- UI 및 상태 보조 함수 ---

void CmfcCircleDrawDlg::UpdateCoordinateDisplay()
{
	CString strPoint;
	for (size_t i = 0; i < 3; ++i)
	{
		UINT nID = 0;
		// 현재까지 클릭된 점의 개수에 따라 좌표를 표시하거나, 기본 텍스트를 표시
		if (i < m_clickPoints.size())
		{
			strPoint.Format(_T("Point %zu: (%d, %d)"), i + 1, m_clickPoints[i].x, m_clickPoints[i].y);
		}
		else
		{
			strPoint.Format(_T("Point %zu:"), i + 1);
		}

		// 인덱스 i에 해당하는 UI 컨트롤 ID를 찾아 텍스트를 설정
		switch (i)
		{
			case 0: nID = IDC_STATIC_POINT1; break;
			case 1: nID = IDC_STATIC_POINT2; break;
			case 2: nID = IDC_STATIC_POINT3; break;
		}
		if (nID != 0) SetDlgItemText(nID, strPoint);
	}
}

int CmfcCircleDrawDlg::GetNearestPointIndex(CPoint point)
{
	// 클릭 허용 오차 범위 15픽셀.
	// 성능을 위해 실제 거리 대신 거리의 제곱을 사용하여 비교
	constexpr int CLICK_THRESHOLD_SQ = 15 * 15;
	for (size_t i = 0; i < m_clickPoints.size(); ++i)
	{
		int dx = point.x - m_clickPoints[i].x;
		int dy = point.y - m_clickPoints[i].y;
		if (dx * dx + dy * dy <= CLICK_THRESHOLD_SQ)
		{
			return static_cast<int>(i); // 클릭된 점의 인덱스 반환
		}
	}
	return -1; // 허용 범위 내에 점이 없음
}

float CmfcCircleDrawDlg::GetCurrentRadius()
{
	CString strRadius;
	GetDlgItemText(IDC_EDIT_RADIUS, strRadius);
	float radius = static_cast<float>(_ttof(strRadius));
	// 유효하지 않은 값 입력했을 경우, 기본값 5.0f를 사용
	return (radius <= 0) ? 5.0f : radius;
}

float CmfcCircleDrawDlg::GetCurrentThickness()
{
	CString strThickness;
	GetDlgItemText(IDC_THICKNESS, strThickness);
	float thickness = static_cast<float>(_ttof(strThickness));
	// 유효하지 않은 값 입력했을 경우, 기본값 2.0f를 사용
	return (thickness <= 0) ? 2.0f : thickness;
}

// --- 애니메이션 스레드 관리 ---

void CmfcCircleDrawDlg::StartAnimationThread()
{
	// 애니메이션이 실행 중, 기존 스레드를 안전하게 종료하고 새로 시작
	if (m_bAnimationRunning)
	{
		StopAnimationThread();
	}

	m_bStopAnimation = false;
	m_bAnimationRunning = true;

	// this 넘겨주어 스레드 함수가 클래스 멤버에 접근할 수 있도록 함.
	// unique_ptr를 사용하여 스레드 객체의 생명주기 관리.
	m_animationThread = std::make_unique<std::thread>(&CmfcCircleDrawDlg::AnimationThreadProc, this);
}

void CmfcCircleDrawDlg::StopAnimationThread()
{
	if (m_animationThread)
	{
		m_bStopAnimation = true; // 스레드 루프에 종료 요청하는 플래그 설정
		m_threadCV.notify_all(); // wait for 상태의 스레드를 즉시 깨움
		if (m_animationThread->joinable())
		{
			m_animationThread->join(); // 스레드가 완전히 종료될 때까지 대기
		}
		m_animationThread.reset(); // 스레드 객체 리소스 해제
	}
	m_bAnimationRunning = false;
	m_bStopAnimation = false; // 다음 실행을 위해 플래그 초기화
}

void CmfcCircleDrawDlg::AnimationThreadProc()
{
	std::cout << "Animation thread started." << std::endl;
	for (int i = 0; i < ANIMATION_COUNT; ++i)
	{
		// 루프 시작 시마다 중지 요청이 있었는지 확인
		if (m_bStopAnimation)
		{
			std::cout << "Animation stopped by user." << std::endl;
			break;
		}

		// 1. 점들을 랜덤 위치로 이동
		RandomMovePoints();

		// 2. 변경된 데이터로 오프스크린 버퍼 m_image 에 다시 그림. ui가 아닌 워킹 스레드에서 그림.
		RedrawScene();

		// 3. UI 프리징 방지를 위해 UI 스레드에 화면 갱신 요청하는 메시지를 보냄
		PostMessage(WM_UPDATE_DISPLAY);

		// 4. 다음 프레임까지 대기. m_bStopAnimation 플래그가 true가 되면 즉시 깨어남.
		std::unique_lock<std::mutex> lock(m_threadMutex);
		if (m_threadCV.wait_for(lock, std::chrono::milliseconds(ANIMATION_DELAY_MS), [this] { return m_bStopAnimation.load(); }))
		{
			// wait_for가 true를 반환하면, 대기 시간이 다 되기 전에 중지 요청으로 깨어났다는 의미
			std::cout << "Animation wait interrupted." << std::endl;
			break;
		}
	}
	m_bAnimationRunning = false; // 애니메이션 종료 상태로 설정
	std::cout << "Animation thread finished." << std::endl;
}

void CmfcCircleDrawDlg::RandomMovePoints()
{
	//스레드 마다 독립적인 난수 생성
	thread_local std::mt19937 gen(std::random_device{}());

	int nWidth = m_image.GetWidth();
	int nHeight = m_image.GetHeight();
	// 원이 화면 밖으로 나가지 않도록 margin 계산
	int margin = static_cast<int>(GetCurrentRadius()) + 5;

	std::uniform_int_distribution<> distX(margin, nWidth - margin);
	std::uniform_int_distribution<> distY(margin, nHeight - margin);

	// m_clickPoints는 UI 스레드와 작업 스레드가 쉐어하는 데이터이므로, 수정하는 동안 뮤텍스로 락을 걸어 race condition을 방지.
	std::lock_guard<std::mutex> lock(m_threadMutex);
	for (auto& point : m_clickPoints)
	{
		point.x = distX(gen);
		point.y = distY(gen);
	}
}