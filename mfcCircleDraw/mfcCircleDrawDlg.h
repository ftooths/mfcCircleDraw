#pragma once

#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <memory>
#include <random>

// 사용자 정의 id. 안전하게 화면 그리기 요청
#define WM_UPDATE_DISPLAY (WM_USER + 101)

class CmfcCircleDrawDlg : public CDialogEx
{

public:
    CmfcCircleDrawDlg(CWnd* pParent = nullptr); // 생성자
    virtual ~CmfcCircleDrawDlg(); // 소멸자


#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_MFCCIRCLEDRAW_DIALOG };
#endif

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();

    // 메세지 핸들러 함수들
    DECLARE_MESSAGE_MAP()

    afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
    afx_msg void OnPaint(); // 화면 다시 그릴 떄 호출되는 함수
    afx_msg HCURSOR OnQueryDragIcon();
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point); // 마우스 왼쪽 버튼 누를 때
    afx_msg void OnMouseMove(UINT nFlags, CPoint point); // 마우스 드래그 할 때
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point); // 마우스 왼쪽 버튼 뗄 때
    afx_msg void OnEnChangeEditRadius(); // radius 값 변경 시 작동
    afx_msg void OnEnChangeThickness(); // 두께 값 변경 시 작동
    afx_msg void OnBnClickedBtnReset(); // 리셋 버튼
    afx_msg void OnBnClickedBtnRandomMove(); // 랜덤 이동
    afx_msg LRESULT OnUpdateDisplay(WPARAM wParam, LPARAM lParam);

private:

    HICON m_hIcon;
    CImage m_image; // 화면에 그려질 모든 내용 담는 이미지 버퍼. 모든 그리기 작업을 해당 객체에서 먼저 수행 후, Onpaint 함수에서 한번에 복사 
    std::vector<CPoint> m_clickPoints; // 사용자 클릭 점 좌표 저장용 벡터 배열

    bool m_bDragging; // 현재 드래그 중인지 판단하는 변수
    int m_nDragIndex; // 드래그 중인 점이 몇번째 점인지 확인하는 용도


    // --- 스레드 관련 함수---

    std::unique_ptr<std::thread> m_animationThread; // 애니메이션 스레드 실행할 작업 스레드 객체 관리용 포인터
    std::mutex m_threadMutex; // UI 스레드와 워킹 스레드가 공유하는 데이터의 동시 접근을 막기 위한 엑세스 제한 뮤텍스. m_clickPoints와 같이 검사
    std::condition_variable m_threadCV; // 워킹 스레드가 500ms 동안 대기, 필요 시 즉시 깨어날 수 있도록 하는 체크용 변수
    std::atomic<bool> m_bStopAnimation; // 애니메이션 스레드 중지를 요청하는 플래그. atomic으로 선언하여 멀티스레드 환경에서 안전하게 접근 가능
    std::atomic<bool> m_bAnimationRunning; // 애니메이션 스레드가 실행 중인지 상태 확인용 플래그

    // 애니메이션 상수
    static constexpr int ANIMATION_COUNT = 10;
    static constexpr int ANIMATION_DELAY_MS = 500; //(500ms, 초당 2회)


    // --- 기타 함수---

    // 초기화
    void InitializeImageBuffer();  //m_image 버퍼를 생성하고 흰색으로 초기화.

    // 원 그리는 함수
    void RedrawScene(); // 현재상태 기반으로 image에 다시 그림
    void DrawFilledCircle(int centerX, int centerY, float radius, unsigned char color = 0); // 검정색으로 채워진 원, 3개 점을 그림
    void DrawHollowCircle(double centerX, double centerY, double radius, float thickness, unsigned char color = 0); // 지정된 두께와 색상으로 속이 빈 원을 그림.
    bool CalculateCircumcircle(CPoint p1, CPoint p2, CPoint p3, double& centerX, double& centerY, double& radius); // 세 점을 지나는 외접원의 중심과 반지름을 계산.

    // UI 및 상태 관리 함수
    void UpdateCoordinateDisplay(); // UI의 좌표를 현재 m_clickPoints 값으로 업데이트
    int GetNearestPointIndex(CPoint point); //  드래그인지, 점에 충분히 가까운 지점을 터치했는지 확인 
    float GetCurrentRadius(); // ui에서 radius 값 읽어오는 함수
    float GetCurrentThickness(); // ui에서 두께 읽어오는 함수

    // 애니메이션 스레드 보조 함수
    void StartAnimationThread(); // 애니메이션 스레드 시작
    void StopAnimationThread(); // 실행 중인 애니메이션 스레드 종료
    void AnimationThreadProc(); // 애니메이션 스레드 실제로 실핸하는 루프 함수
    void RandomMovePoints(); // 세 개의 클릭 지점 랜덤으로 이동시키는 함수
};
