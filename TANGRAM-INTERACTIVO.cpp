// Tangram interactivo en OpenGL con GLEW y GLFW
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cmath>
#include <vector>
#include <iostream>

struct Piece {
    std::vector<std::pair<float, float>> vertices; // Vertices relativos al centro
    float x, y;       // Centro de la pieza
    float angle;      // Rotación en grados
    float r, g, b;    // Color
};

void drawPiece(const Piece& p) {
    glPushMatrix();
    glTranslatef(p.x, p.y, 0);
    glRotatef(p.angle, 0, 0, 1);
    glColor3f(p.r, p.g, p.b);

    glBegin(GL_POLYGON);
    for (auto v : p.vertices)
        glVertex2f(v.first, v.second);
    glEnd();

    glPopMatrix();
}

float rad(float deg) {
    return deg * M_PI / 180.0f;
}

int main() {
    if (!glfwInit()) {
        std::cerr << "No se pudo inicializar GLFW\n";
        return -1;
    }
    GLFWwindow* window = glfwCreateWindow(800, 800, "Tangram Interactivo - OpenGL", NULL, NULL);
    if (!window) {
        std::cerr << "No se pudo crear la ventana\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glewExperimental = true;
    if (glewInit() != GLEW_OK) {
        std::cerr << "No se pudo inicializar GLEW\n";
        return -1;
    }

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, 800, 0, 800, -1, 1);
    glMatrixMode(GL_MODELVIEW);

    // Define las 7 piezas del tangram (triángulos, cuadrado, paralelogramo)
    std::vector<Piece> pieces = {
        // Triángulo grande 1
        {{{0,0},{200,0},{0,200}}, 200,600,0, 1,0,0},
        // Triángulo grande 2
        {{{0,0},{200,0},{0,200}}, 600,600,0, 0,1,0},
        // Triángulo mediano
        {{{0,0},{100,100},{0,200}}, 600,400,0, 0,0,1},
        // Triángulo pequeño 1
        {{{0,0},{100,0},{0,100}}, 200,400,0, 1,1,0},
        // Triángulo pequeño 2
        {{{0,0},{100,0},{0,100}}, 400,200,0, 1,0,1},
        // Cuadrado
        {{{0,0},{100,0},{100,100},{0,100}}, 400,400,0, 0,1,1},
        // Paralelogramo
        {{{0,0},{100,50},{50,150},{-50,100}}, 600,200,0, 0.5,0.2,0.6}
    };

    int selected = 0; // Índice de la pieza seleccionada

    std::cout << "Controles:\n";
    std::cout << "Flechas: mover pieza seleccionada\n";
    std::cout << "Z/X: rotar pieza seleccionada\n";
    std::cout << "TAB: cambiar pieza seleccionada\n";

    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.95f, 0.95f, 0.95f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Dibujar todas las piezas
        for (int i = 0; i < pieces.size(); ++i) {
            if (i == selected) {
                glLineWidth(4);
                glColor3f(0,0,0);
                glBegin(GL_LINE_LOOP);
                glPushMatrix();
                glTranslatef(pieces[i].x, pieces[i].y, 0);
                glRotatef(pieces[i].angle, 0, 0, 1);
                for (auto v : pieces[i].vertices)
                    glVertex2f(v.first, v.second);
                glPopMatrix();
                glEnd();
            }
            drawPiece(pieces[i]);
        }

        // Controles de teclado
        if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS) {
            selected = (selected + 1) % pieces.size();
            glfwWaitEventsTimeout(0.15);
        }
        Piece& p = pieces[selected];
        
        // MODIFICACIÓN: Movimiento más lento (2 unidades por pulsación)
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) p.x -= 2;
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) p.x += 2;
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) p.y += 2;
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) p.y -= 2;

        // MODIFICACIÓN: Rotación más lenta (1 grado por pulsación)
        if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) {
            p.angle += 1;
            glfwWaitEventsTimeout(0.02);
        }
        if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) {
            p.angle -= 1;
            glfwWaitEventsTimeout(0.02);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}