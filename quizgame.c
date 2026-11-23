/*
 * C Quiz Game - graphical version (Windows)
 * No download needed. Uses the Windows GUI that is already on your PC.
 *
 * Compile (PowerShell):
 *   gcc quiz_gui.c -o quiz_gui.exe -mwindows -lgdi32 -lmsimg32
 * Run:
 *   .\quiz_gui.exe
 */

#include <windows.h>
#include <stdio.h>

#define WIN_W        900
#define WIN_H        650
#define NUM_OPTIONS  4
#define TIME_PER_Q   15
#define TIMER_ID     1

typedef struct {
    const char *question;
    const char *options[NUM_OPTIONS];
    int correct;                       /* index 0-3 of the right option */
} Question;

/* ---- Add or edit your questions here (keep them short) ---- */
static Question questions[] = {
    { "Who created the C language?",
      { "Dennis Ritchie", "James Gosling", "Bjarne Stroustrup", "Guido van Rossum" }, 0 },
    { "Which symbol gives the address of a variable?",
      { "*", "&", "#", "@" }, 1 },
    { "Which header file is needed for printf()?",
      { "stdlib.h", "string.h", "stdio.h", "math.h" }, 2 },
    { "What is the size of int on most modern systems?",
      { "1 byte", "2 bytes", "4 bytes", "16 bytes" }, 2 },
    { "Which loop runs at least once?",
      { "for", "while", "do-while", "foreach" }, 2 },
    { "Which function allocates memory dynamically?",
      { "malloc()", "alloc_mem()", "new()", "create()" }, 0 },
    { "What is the index of the first element in an array?",
      { "1", "0", "-1", "Depends" }, 1 },
    { "Which keyword is used to exit a loop early?",
      { "stop", "exit", "break", "return loop" }, 2 }
};
#define TOTAL ((int)(sizeof(questions) / sizeof(questions[0])))

typedef enum { MENU, PLAYING, FEEDBACK, RESULT } State;

/* ---- Colors ---- */
#define C_BG_TOP     RGB(30, 27, 75)
#define C_BG_BOTTOM  RGB(88, 28, 135)
#define C_CARD       RGB(48, 38, 108)
#define C_CARD_EDGE  RGB(125, 100, 205)
#define C_BTN        RGB(70, 52, 135)
#define C_BTN_HOT    RGB(112, 84, 195)
#define C_ACCENT     RGB(250, 204, 21)
#define C_ACCENT_HOT RGB(253, 224, 71)
#define C_GOOD       RGB(34, 197, 94)
#define C_BAD        RGB(239, 68, 68)
#define C_WHITE      RGB(255, 255, 255)
#define C_SOFT       RGB(200, 190, 240)
#define C_DARK       RGB(40, 30, 80)

/* ---- Game data ---- */
static State state = MENU;
static int current = 0, score = 0, correctCount = 0;
static int selected = -1, lastPoints = 0, hover = -1;
static DWORD qStart = 0;

static HFONT fTitle, fBig, fMed, fSmall;

static RECT StartRect = { 330, 440, 570, 510 };

static RECT OptionRect(int i)
{
    RECT r;
    r.left = 100;  r.right = 800;
    r.top = 240 + i * 85;  r.bottom = r.top + 68;
    return r;
}

static int TimeLeftMs(void)
{
    int left = TIME_PER_Q * 1000 - (int)(GetTickCount() - qStart);
    return left < 0 ? 0 : left;
}

/* ---- Drawing helpers ---- */

static void GradientV(HDC dc, int x0, int y0, int x1, int y1, COLORREF c1, COLORREF c2)
{
    TRIVERTEX v[2];
    GRADIENT_RECT gr;

    v[0].x = x0; v[0].y = y0;
    v[0].Red = GetRValue(c1) << 8; v[0].Green = GetGValue(c1) << 8;
    v[0].Blue = GetBValue(c1) << 8; v[0].Alpha = 0;
    v[1].x = x1; v[1].y = y1;
    v[1].Red = GetRValue(c2) << 8; v[1].Green = GetGValue(c2) << 8;
    v[1].Blue = GetBValue(c2) << 8; v[1].Alpha = 0;
    gr.UpperLeft = 0; gr.LowerRight = 1;
    GradientFill(dc, v, 2, &gr, 1, GRADIENT_FILL_RECT_V);
}

static void RoundBox(HDC dc, RECT r, COLORREF fill, COLORREF border, int radius)
{
    HBRUSH b = CreateSolidBrush(fill);
    HPEN p = CreatePen(PS_SOLID, 2, border);
    HGDIOBJ ob = SelectObject(dc, b);
    HGDIOBJ op = SelectObject(dc, p);
    RoundRect(dc, r.left, r.top, r.right, r.bottom, radius, radius);
    SelectObject(dc, ob);
    SelectObject(dc, op);
    DeleteObject(b);
    DeleteObject(p);
}

static void Circle(HDC dc, int x, int y, int d, COLORREF fill)
{
    HBRUSH b = CreateSolidBrush(fill);
    HPEN p = CreatePen(PS_SOLID, 1, fill);
    HGDIOBJ ob = SelectObject(dc, b);
    HGDIOBJ op = SelectObject(dc, p);
    Ellipse(dc, x, y, x + d, y + d);
    SelectObject(dc, ob);
    SelectObject(dc, op);
    DeleteObject(b);
    DeleteObject(p);
}

static void Text(HDC dc, const char *s, RECT r, HFONT f, COLORREF c, UINT fmt)
{
    SelectObject(dc, f);
    SetTextColor(dc, c);
    DrawTextA(dc, s, -1, &r, fmt | DT_SINGLELINE | DT_NOPREFIX);
}

static void DrawButton(HDC dc, RECT r, const char *label, int hot)
{
    RoundBox(dc, r, hot ? C_ACCENT_HOT : C_ACCENT, C_WHITE, 34);
    Text(dc, label, r, fMed, C_DARK, DT_CENTER | DT_VCENTER);
}

static HFONT MakeFont(int height, int weight)
{
    return CreateFontA(-height, 0, 0, 0, weight, FALSE, FALSE, FALSE,
                       DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                       ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_SWISS, "Segoe UI");
}

/* ---- Screens ---- */

static void DrawMenu(HDC dc)
{
    RECT card = { 150, 60, 750, 590 };
    RECT r;

    RoundBox(dc, card, C_CARD, C_CARD_EDGE, 44);

    r.left = 0; r.right = WIN_W;
    r.top = 100; r.bottom = 180;
    Text(dc, "C PROGRAMMING", r, fTitle, C_ACCENT, DT_CENTER | DT_VCENTER);
    r.top = 180; r.bottom = 260;
    Text(dc, "QUIZ GAME", r, fTitle, C_WHITE, DT_CENTER | DT_VCENTER);

    r.top = 275; r.bottom = 315;
    Text(dc, "Test your knowledge of the C language!", r, fMed, C_SOFT, DT_CENTER | DT_VCENTER);

    r.top = 335; r.bottom = 365;
    Text(dc, "Answer with the mouse or keys A, B, C, D", r, fSmall, C_WHITE, DT_CENTER | DT_VCENTER);
    r.top = 365; r.bottom = 395;
    Text(dc, "15 seconds for each question", r, fSmall, C_WHITE, DT_CENTER | DT_VCENTER);
    r.top = 395; r.bottom = 425;
    Text(dc, "Faster answers give more points", r, fSmall, C_WHITE, DT_CENTER | DT_VCENTER);

    DrawButton(dc, StartRect, "START", hover == 0);

    r.top = 525; r.bottom = 555;
    Text(dc, "or press ENTER", r, fSmall, C_SOFT, DT_CENTER | DT_VCENTER);
}

static void DrawQuiz(HDC dc)
{
    const Question *q = &questions[current];
    char buf[128];
    RECT r;
    int i;

    /* Header */
    sprintf(buf, "Question %d / %d", current + 1, TOTAL);
    r.left = 50; r.right = 450; r.top = 20; r.bottom = 65;
    Text(dc, buf, r, fMed, C_WHITE, DT_LEFT | DT_VCENTER);

    sprintf(buf, "Score: %d", score);
    r.left = 450; r.right = 850;
    Text(dc, buf, r, fMed, C_ACCENT, DT_RIGHT | DT_VCENTER);

    /* Timer bar */
    {
        RECT back = { 50, 75, 850, 95 };
        int ms = (state == PLAYING) ? TimeLeftMs() : 0;
        double ratio = (double)ms / (TIME_PER_Q * 1000.0);
        RoundBox(dc, back, RGB(30, 22, 80), RGB(30, 22, 80), 20);
        if (ratio > 0.02)
        {
            RECT fill = back;
            fill.right = back.left + (int)(800 * ratio);
            RoundBox(dc, fill, ratio > 0.3 ? C_GOOD : C_BAD,
                               ratio > 0.3 ? C_GOOD : C_BAD, 20);
        }
    }

    /* Question card */
    {
        RECT card = { 60, 120, 840, 215 };
        RoundBox(dc, card, C_CARD, C_CARD_EDGE, 30);
        Text(dc, q->question, card, fMed, C_WHITE, DT_CENTER | DT_VCENTER);
    }

    /* Options */
    for (i = 0; i < NUM_OPTIONS; i++)
    {
        RECT o = OptionRect(i);
        COLORREF fill = (hover == i && state == PLAYING) ? C_BTN_HOT : C_BTN;
        COLORREF edge = C_CARD_EDGE;
        RECT t;
        char letter[2];

        if (state == FEEDBACK)
        {
            if (i == q->correct)      { fill = C_GOOD; edge = C_WHITE; }
            else if (i == selected)   { fill = C_BAD;  edge = C_WHITE; }
        }
        RoundBox(dc, o, fill, edge, 30);

        Circle(dc, o.left + 14, o.top + 14, 40, C_ACCENT);
        letter[0] = (char)('A' + i); letter[1] = 0;
        t.left = o.left + 14; t.top = o.top + 14;
        t.right = t.left + 40; t.bottom = t.top + 40;
        Text(dc, letter, t, fMed, C_DARK, DT_CENTER | DT_VCENTER);

        t = o;
        t.left = o.left + 75;
        Text(dc, q->options[i], t, fMed, C_WHITE, DT_LEFT | DT_VCENTER);
    }

    /* Feedback message */
    if (state == FEEDBACK)
    {
        const char *msg;
        COLORREF col;
        RECT m = { 100, 580, 520, 630 };
        RECT h = { 450, 580, 800, 630 };

        if (selected == q->correct)
        {
            sprintf(buf, "Correct!  +%d points", lastPoints);
            msg = buf; col = C_GOOD;
        }
        else if (selected == -1) { msg = "Time's up!";    col = C_ACCENT; }
        else                     { msg = "Wrong answer!"; col = C_BAD; }

        Text(dc, msg, m, fMed, col, DT_LEFT | DT_VCENTER);
        Text(dc, "Click or press SPACE to continue", h, fSmall, C_SOFT, DT_RIGHT | DT_VCENTER);
    }
}

static void DrawResult(HDC dc)
{
    RECT card = { 150, 60, 750, 590 };
    RECT r;
    char buf[128];
    const char *rating;

    RoundBox(dc, card, C_CARD, C_CARD_EDGE, 44);

    r.left = 0; r.right = WIN_W;
    r.top = 100; r.bottom = 180;
    Text(dc, "QUIZ COMPLETE!", r, fTitle, C_ACCENT, DT_CENTER | DT_VCENTER);

    sprintf(buf, "Score: %d", score);
    r.top = 200; r.bottom = 260;
    Text(dc, buf, r, fBig, C_WHITE, DT_CENTER | DT_VCENTER);

    sprintf(buf, "Correct answers: %d / %d  (%d%%)",
            correctCount, TOTAL, correctCount * 100 / TOTAL);
    r.top = 275; r.bottom = 315;
    Text(dc, buf, r, fMed, C_SOFT, DT_CENTER | DT_VCENTER);

    if (correctCount == TOTAL)               rating = "Perfect! You are a C master!";
    else if (correctCount * 10 >= TOTAL * 6) rating = "Great job! Keep practising.";
    else                                     rating = "Keep learning, you will get there!";
    r.top = 335; r.bottom = 385;
    Text(dc, rating, r, fMed, C_GOOD, DT_CENTER | DT_VCENTER);

    DrawButton(dc, StartRect, "PLAY AGAIN", hover == 0);

    r.top = 525; r.bottom = 555;
    Text(dc, "Press R to play again  |  ESC to quit", r, fSmall, C_SOFT, DT_CENTER | DT_VCENTER);
}

static void DrawScene(HDC dc)
{
    GradientV(dc, 0, 0, WIN_W, WIN_H, C_BG_TOP, C_BG_BOTTOM);
    SetBkMode(dc, TRANSPARENT);

    if (state == MENU)                          DrawMenu(dc);
    else if (state == PLAYING || state == FEEDBACK) DrawQuiz(dc);
    else                                        DrawResult(dc);
}

/* ---- Game logic ---- */

static void StartGame(void)
{
    current = 0; score = 0; correctCount = 0;
    selected = -1; hover = -1;
    qStart = GetTickCount();
    state = PLAYING;
}

static void Answer(int choice)
{
    selected = choice;
    if (choice == questions[current].correct)
    {
        lastPoints = 10 + TimeLeftMs() / 1000;
        score += lastPoints;
        correctCount++;
    }
    hover = -1;
    state = FEEDBACK;
}

static void NextQuestion(void)
{
    current++;
    selected = -1;
    hover = -1;
    if (current >= TOTAL) state = RESULT;
    else { state = PLAYING; qStart = GetTickCount(); }
}

static void UpdateHover(HWND h, POINT p)
{
    int nh = -1, i;

    if (state == MENU || state == RESULT)
    {
        if (PtInRect(&StartRect, p)) nh = 0;
    }
    else if (state == PLAYING)
    {
        for (i = 0; i < NUM_OPTIONS; i++)
        {
            RECT r = OptionRect(i);
            if (PtInRect(&r, p)) nh = i;
        }
    }
    if (nh != hover) { hover = nh; InvalidateRect(h, NULL, FALSE); }
}

/* ---- Window procedure ---- */

static LRESULT CALLBACK WndProc(HWND h, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_CREATE:
        fTitle = MakeFont(56, FW_BOLD);
        fBig   = MakeFont(44, FW_BOLD);
        fMed   = MakeFont(26, FW_SEMIBOLD);
        fSmall = MakeFont(20, FW_NORMAL);
        SetTimer(h, TIMER_ID, 40, NULL);
        return 0;

    case WM_TIMER:
        if (state == PLAYING)
        {
            if (TimeLeftMs() <= 0) Answer(-1);
            InvalidateRect(h, NULL, FALSE);
        }
        return 0;

    case WM_MOUSEMOVE:
    {
        POINT p;
        p.x = (short)LOWORD(lp);
        p.y = (short)HIWORD(lp);
        UpdateHover(h, p);
        return 0;
    }

    case WM_SETCURSOR:
        if (LOWORD(lp) == HTCLIENT && hover != -1)
        {
            SetCursor(LoadCursor(NULL, IDC_HAND));
            return TRUE;
        }
        break;

    case WM_LBUTTONDOWN:
    {
        POINT p;
        p.x = (short)LOWORD(lp);
        p.y = (short)HIWORD(lp);

        if ((state == MENU || state == RESULT) && PtInRect(&StartRect, p))
            StartGame();
        else if (state == PLAYING)
        {
            int i;
            for (i = 0; i < NUM_OPTIONS; i++)
            {
                RECT r = OptionRect(i);
                if (PtInRect(&r, p)) { Answer(i); break; }
            }
        }
        else if (state == FEEDBACK)
            NextQuestion();
        InvalidateRect(h, NULL, FALSE);
        return 0;
    }

    case WM_KEYDOWN:
        if (wp == VK_ESCAPE) { DestroyWindow(h); return 0; }

        if (state == MENU && wp == VK_RETURN)
            StartGame();
        else if (state == PLAYING)
        {
            if (wp >= '1' && wp <= '4')      Answer((int)(wp - '1'));
            else if (wp >= 'A' && wp <= 'D') Answer((int)(wp - 'A'));
        }
        else if (state == FEEDBACK && (wp == VK_SPACE || wp == VK_RETURN))
            NextQuestion();
        else if (state == RESULT && (wp == 'R' || wp == VK_RETURN))
            StartGame();

        InvalidateRect(h, NULL, FALSE);
        return 0;

    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(h, &ps);
        HDC mem = CreateCompatibleDC(dc);
        HBITMAP bmp = CreateCompatibleBitmap(dc, WIN_W, WIN_H);
        HGDIOBJ old = SelectObject(mem, bmp);

        DrawScene(mem);                                   /* draw off-screen */
        BitBlt(dc, 0, 0, WIN_W, WIN_H, mem, 0, 0, SRCCOPY); /* then copy: no flicker */

        SelectObject(mem, old);
        DeleteObject(bmp);
        DeleteDC(mem);
        EndPaint(h, &ps);
        return 0;
    }

    case WM_DESTROY:
        KillTimer(h, TIMER_ID);
        DeleteObject(fTitle);
        DeleteObject(fBig);
        DeleteObject(fMed);
        DeleteObject(fSmall);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcA(h, msg, wp, lp);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR cmd, int show)
{
    WNDCLASSEXA wc;
    RECT rc = { 0, 0, WIN_W, WIN_H };
    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    HWND hwnd;
    MSG m;
    int x, y;

    (void)hPrev; (void)cmd;

    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = "CQuizWindow";
    RegisterClassExA(&wc);

    AdjustWindowRect(&rc, style, FALSE);
    x = (GetSystemMetrics(SM_CXSCREEN) - (rc.right - rc.left)) / 2;
    y = (GetSystemMetrics(SM_CYSCREEN) - (rc.bottom - rc.top)) / 2;
    if (y < 0) y = 0;

    hwnd = CreateWindowExA(0, "CQuizWindow", "C Programming Quiz", style,
                           x, y, rc.right - rc.left, rc.bottom - rc.top,
                           NULL, NULL, hInst, NULL);
    ShowWindow(hwnd, show);
    UpdateWindow(hwnd);

    while (GetMessage(&m, NULL, 0, 0) > 0)
    {
        TranslateMessage(&m);
        DispatchMessage(&m);
    }
    return (int)m.wParam;
}

