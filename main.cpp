#include <GL/glut.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// --- SOUND INCLUDES (Windows Only) ---
#ifdef _WIN32
#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib") // Link the Windows Multimedia library
#endif

// --- GLOBAL VARIABLES (Ref: Section2.pdf) ---
int phyWidth = 700;
int phyHeight = 700;
int logWidth = 100;
int logHeight = 100;

// Game State: 0 = Menu, 1 = Playing, 2 = Game Over
int gameState = 0;

// Game Logic
int score = 0;
float timeLeft = 60.0f; // 60 seconds song
int currentLane = 0;    // -1 = Left, 0 = Center, 1 = Right
float laneWidth = 2.5f; // Distance between lanes

// Note/Object Structure
struct Note {
    float z;       // Position on highway
    int lane;      // Which lane it is in (-1, 0, 1)
    bool active;   // Is it visible?
    int type;      // 0 = Cube (Point), 1 = Sphere (Bonus)
};

#define MAX_NOTES 20
Note notes[MAX_NOTES];
float gameSpeed = 0.7f; // Base speed (Fast Version)

// --- FUNCTION PROTOTYPES ---
void init();
void display();
void resize(int w, int h);
void specialKeyboard(int key, int x, int y); // Ref: Keyboard.pdf
void mouseClick(int button, int state, int x, int y); // Ref: mouse1.pdf
void timer(int value);
void drawText(const char* str, float x, float y); // Ref: Section3.pdf
void playSoundEffect(const char* filename); // New helper for sound

// --- HELPER FUNCTIONS ---

// Switch to 2D for Menu/HUD (Ref: Section2.pdf logic)
void setOrthographic() {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, logWidth, 0, logHeight);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glDisable(GL_DEPTH_TEST);
}

// Switch back to 3D for Gameplay (Bonus Requirement)
void setPerspective() {
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_DEPTH_TEST);
}

// Text Drawing Routine (Ref: Section3.pdf page 16)
void drawText(const char* str, float x, float y) {
    glRasterPos2f(x, y);
    int len = (int)strlen(str);
    for (int i = 0; i < len; i++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, str[i]);
    }
}

// Sound Helper Function
void playSoundEffect(const char* filename) {
#ifdef _WIN32
    // SND_ASYNC: Play sound in background (don't freeze game)
    // SND_FILENAME: The parameter is a filename
    PlaySound(TEXT(filename), NULL, SND_ASYNC | SND_FILENAME);
#endif
}

// --- DRAWING FUNCTIONS ---

// Draw the "Cyber Catcher" Player (Requirement: At least 3 Shapes)
void drawPlayer() {
    glPushMatrix();
    // Position player based on current lane
    glTranslatef(currentLane * laneWidth, 0.0f, 0.0f);

    // Shape 1: The Base Plate (Box)
    glColor3f(0.2, 0.2, 0.2); // Dark Grey
    glPushMatrix();
    glScalef(1.5, 0.2, 1.0);
    glutSolidCube(1.0);
    glPopMatrix();

    // Shape 2 & 3: Side "Magnets" (Cones)
    glColor3f(0.0, 1.0, 1.0); // Cyan Neon

    // Left Magnet
    glPushMatrix();
    glTranslatef(-0.8, 0.5, 0.0);
    glRotatef(-90, 1, 0, 0);
    glutSolidCone(0.2, 0.8, 10, 10);
    glPopMatrix();

    // Right Magnet
    glPushMatrix();
    glTranslatef(0.8, 0.5, 0.0);
    glRotatef(-90, 1, 0, 0);
    glutSolidCone(0.2, 0.8, 10, 10);
    glPopMatrix();

    // Shape 4: Energy Core (Sphere)
    glColor3f(1.0, 0.0, 1.0); // Magenta
    glPushMatrix();
    glTranslatef(0.0, 0.5, 0.0);
    glutWireSphere(0.3, 10, 10);
    glPopMatrix();

    glPopMatrix();
}

// Draw the Rhythm Notes
void drawNotes() {
    for (int i = 0; i < MAX_NOTES; i++) {
        if (notes[i].active) {
            glPushMatrix();
            // Move to lane and Z position
            glTranslatef(notes[i].lane * laneWidth, 0.5f, notes[i].z);

            if(notes[i].type == 0) {
                glColor3f(0.0, 1.0, 0.0); // Green Note
                glutSolidCube(0.8);
            } else {
                glColor3f(1.0, 1.0, 0.0); // Gold Bonus Note
                glutSolidSphere(0.5, 15, 15);
            }
            glPopMatrix();
        }
    }
}

// Draw the 3D Highway (Requirement: Patterned Background)
void drawHighway() {
    // Draw 3 Lanes
    float zFar = -50.0f;
    float zNear = 10.0f;

    // Lane dividers
    glLineWidth(3.0);
    glBegin(GL_LINES);
    glColor3f(1.0, 1.0, 1.0); // White Lines

    // Left Line
    glVertex3f(-laneWidth * 1.5, 0, zNear);
    glVertex3f(-laneWidth * 1.5, 0, zFar);

    // Mid-Left Line
    glVertex3f(-laneWidth * 0.5, 0, zNear);
    glVertex3f(-laneWidth * 0.5, 0, zFar);

    // Mid-Right Line
    glVertex3f(laneWidth * 0.5, 0, zNear);
    glVertex3f(laneWidth * 0.5, 0, zFar);

    // Right Line
    glVertex3f(laneWidth * 1.5, 0, zNear);
    glVertex3f(laneWidth * 1.5, 0, zFar);
    glEnd();

    // Draw "Neon" Grid on the floor
    glColor3f(0.1, 0.0, 0.3); // Dark Purple floor
    glBegin(GL_QUADS);
    glVertex3f(-10, -0.1, zNear);
    glVertex3f(10, -0.1, zNear);
    glVertex3f(10, -0.1, zFar);
    glVertex3f(-10, -0.1, zFar);
    glEnd();
}

void drawMenu() {
    setOrthographic(); // Switch to 2D for menu

    // Title
    glColor3f(0.0, 1.0, 1.0); // Cyan
    drawText("NEON RHYTHM HIGHWAY", 32, 80);

    // Start Button (Green Quad)
    glColor3f(0.0, 0.8, 0.0);
    glBegin(GL_QUADS);
    glVertex2f(35, 50); glVertex2f(65, 50);
    glVertex2f(65, 65); glVertex2f(35, 65);
    glEnd();
    glColor3f(1.0, 1.0, 1.0);
    drawText("START DRIVE", 40, 56);

    // Exit Button (Red Quad)
    glColor3f(0.8, 0.0, 0.0);
    glBegin(GL_QUADS);
    glVertex2f(35, 25); glVertex2f(65, 25);
    glVertex2f(65, 40); glVertex2f(35, 40);
    glEnd();
    glColor3f(1.0, 1.0, 1.0);
    drawText("EXIT", 46, 31);

    setPerspective(); // Switch back
}

void drawHUD() {
    setOrthographic();

    glColor3f(1.0, 1.0, 0.0); // Yellow Text
    char buffer[50];

    // Score Display (Top Left)
    sprintf(buffer, "SCORE: %d", score);
    drawText(buffer, 5, 92);

    // Timer Display (Top Right)
    if (gameState == 1) {
        sprintf(buffer, "TIME: %.1f", timeLeft);
        drawText(buffer, 75, 92);

        // Show Speed Level
        glColor3f(1.0, 0.0, 1.0);
        sprintf(buffer, "SPEED: %.1fx", (gameSpeed/0.7f)); // Display relative speed
        drawText(buffer, 75, 88);
    } else if (gameState == 2) {
        glColor3f(1.0, 0.0, 0.0);
        drawText("RUN COMPLETE!", 40, 50);
        sprintf(buffer, "Final Score: %d", score);
        drawText(buffer, 42, 45);
        drawText("Right Click for Menu", 38, 35);
    }

    setPerspective();
}

// --- MAIN DISPLAY FUNCTION ---
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // 3D Camera Position (Above player, looking down highway)
    gluLookAt(0.0, 4.0, 12.0,   // Eye
              0.0, 0.0, -10.0,  // Center
              0.0, 1.0, 0.0);   // Up

    if (gameState == 0) {
        drawMenu();
    } else {
        drawHighway();
        drawPlayer();
        drawNotes();
        drawHUD();
    }

    glutSwapBuffers();
}

// --- INPUT HANDLING ---

// Mouse Logic (Ref: mouse1.pdf Page 9/21)
void mouseClick(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN && gameState == 0) {
        // Coordinate Conversion logic from PDF
        float mx = (float)x / phyWidth * logWidth;
        float my = (float)(phyHeight - y) / phyHeight * logHeight;

        // Start Button Check (35-65 X, 50-65 Y)
        if (mx >= 35 && mx <= 65 && my >= 50 && my <= 65) {
            gameState = 1;
            score = 0;
            timeLeft = 60.0;
            gameSpeed = 0.7f; // Reset Speed (Starts Faster now)
            // Reset notes
            for(int i=0; i<MAX_NOTES; i++) notes[i].active = false;
        }
        // Exit Button Check (35-65 X, 25-40 Y)
        else if (mx >= 35 && mx <= 65 && my >= 25 && my <= 40) {
            exit(0);
        }
    }
    // Right click to return to menu from Game Over
    if (button == GLUT_RIGHT_BUTTON && gameState == 2) {
        gameState = 0;
    }
    glutPostRedisplay();
}

// Special Keys for Lane Switching (Ref: Keyboard.pdf Page 9)
void specialKeyboard(int key, int x, int y) {
    if (gameState == 1) {
        if (key == GLUT_KEY_LEFT) {
            if (currentLane > -1) currentLane--; // Move Left
        }
        else if (key == GLUT_KEY_RIGHT) {
            if (currentLane < 1) currentLane++;  // Move Right
        }
    }
    glutPostRedisplay();
}

// --- GAME LOGIC ---

void spawnNote() {
    // Find an inactive note
    for (int i = 0; i < MAX_NOTES; i++) {
        if (!notes[i].active) {
            notes[i].active = true;
            notes[i].z = -50.0f; // Start far away
            notes[i].lane = (rand() % 3) - 1; // Random lane: -1, 0, or 1
            notes[i].type = (rand() % 5 == 0) ? 1 : 0; // 20% chance for bonus
            break;
        }
    }
}

void timer(int value) {
    if (gameState == 1) {
        // 1. Timer Countdown
        timeLeft -= 0.016f;
        if (timeLeft <= 0) {
            gameState = 2;
            playSoundEffect("gameover.wav"); // Play Game Over Sound
        }

        // --- DYNAMIC DIFFICULTY ADJUSTMENT ---
        // Increase speed as time decreases
        // Formula: Base speed 0.7 + (Time Elapsed * multiplier)
        gameSpeed = 0.7f + (60.0f - timeLeft) * 0.05f; // Speed increases faster now

        // Increase Spawn Rate (make frequency smaller)
        // Base rate 15 (faster start), decreases to 3
        int spawnRate = 15 - (int)((60.0f - timeLeft) * 0.2f);
        if (spawnRate < 3) spawnRate = 3; // Cap max spawn rate
        // -------------------------------------

        if (rand() % spawnRate == 0) spawnNote();

        // 3. Update Notes
        for (int i = 0; i < MAX_NOTES; i++) {
            if (notes[i].active) {
                notes[i].z += gameSpeed; // Move towards camera using dynamic speed

                // Collision Detection (Player is at Z = 0.0)
                if (notes[i].z >= -0.5f && notes[i].z <= 0.5f) {
                    if (notes[i].lane == currentLane) {
                        // CAUGHT!
                        score += (notes[i].type == 1) ? 50 : 10; // Bonus=50, Normal=10
                        notes[i].active = false; // Hide note
                        playSoundEffect("score.wav"); // Play Score Sound
                    }
                }

                // Missed Note
                if (notes[i].z > 2.0f) {
                    notes[i].active = false;
                }
            }
        }
    }
    glutPostRedisplay();
    glutTimerFunc(16, timer, 0); // 60 FPS loop
}

// Initialization
void init() {
    glClearColor(0.0, 0.0, 0.0, 1.0); // Black Space Background
    glEnable(GL_DEPTH_TEST); // CRITICAL for 3D
}

void resize(int w, int h) {
    phyWidth = w;
    phyHeight = h;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    // 3D Perspective Setup (FOV, Aspect, Near, Far)
    gluPerspective(45.0, (float)w / h, 1.0, 100.0);
    glMatrixMode(GL_MODELVIEW);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH); // Enable Depth Buffer
    glutInitWindowSize(700, 700);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Neon Rhythm Highway - Team Project");

    init();

    glutDisplayFunc(display);
    glutReshapeFunc(resize);
    glutSpecialFunc(specialKeyboard); // Arrow Keys
    glutMouseFunc(mouseClick);        // Mouse Menu
    glutTimerFunc(0, timer, 0);       // Game Loop

    glutMainLoop();
    return 0;
}
