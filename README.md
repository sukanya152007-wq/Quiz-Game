# C Programming Quiz Game

A Windows quiz game written in C using the Windows API. It tests your knowledge of the C language with timed multiple-choice questions.

## Team
- Team ID: 82GMC23B
- Members: [Kritika], [Sukanya], [Vaishnavi]
- Mentor: Ms. Vandana Bhatt
- Institute: Madhav Institute of Technology and Science, Gwalior

## Features
- Start screen with game instructions
- Answer with the mouse or with the keys A, B, C, D
- 15-second timer for every question
- Faster correct answers give more points
- Score and result shown at the end

## Technologies Used
- Language: C
- Windows API (`windows.h`)
- Compiler: GCC (MinGW)
- Editor: Visual Studio Code

## How to Run
1. Download `QuizGame.exe` from this repository (or from the Releases page).
2. Double-click the file to start the game.
3. Click START or press ENTER.

## How to Compile
Open a terminal in the project folder and run:

`gcc quizgame.c -o QuizGame.exe -lgdi32 -lcomctl32 -lwinmm -lmsimg32 -mwindows`

## How to Play
1. Read the question and the four options.
2. Choose an option within 15 seconds, using the mouse or the keys A, B, C, D.
3. The faster you answer correctly, the more points you get.
4. At the end of the quiz, your final score is displayed.

## Files
- `quizgame.c`: source code
- `QuizGame.exe`: compiled Windows application
- `docs/`: test cases and project report

## Author
[Sukanya], Roll No. BTEL26O1114
B.Tech Electronics Engineering, Madhav Institute of Technology and Science