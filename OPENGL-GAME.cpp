// Juego interactivo en OpenGL: Evita el cuadrado
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cstdlib>
#include <ctime>

struct Square {
    float x, y;
    float size;
    float speed_x, speed_y;
};

void drawSquare(float x, float y, float size, float r, float g, float b) {
    glColor3f(r, g, b);
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + size, y);
    glVertex2f(x + size, y + size);
    glVertex2f(x, y + size);
    glEnd();
}

bool isCollision(const Square& a, const Square& b) {
    return (
        a.x < b.x + b.size &&
        a.x + a.size > b.x &&
        a.y < b.y + b.size &&
        a.y + a.size > b.y
    );
}

int main() {
    srand(time(0));
    if (!glfwInit()) {
        std::cerr << "Error inicializando GLFW\n";
        return -1;
    }
    GLFWwindow* window = glfwCreateWindow(600, 600, "Evita el cuadrado - OpenGL", NULL, NULL);
    if (!window) {
        std::cerr << "Error creando ventana\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glewExperimental = true;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Error inicializando GLEW\n";
        return -1;
    }

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, 600, 0, 600, -1, 1);
    glMatrixMode(GL_MODELVIEW);

    Square player = {300, 300, 50, 0, 0};
    Square enemy = {rand() % 550, rand() % 550, 50, 3, 2};
    int score = 0;

    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT);

        // Movimiento del jugador
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) player.x -= 5;
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) player.x += 5;
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) player.y += 5;
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) player.y -= 5;

        // Mantener dentro de la ventana
        if (player.x < 0) player.x = 0;
        if (player.x > 550) player.x = 550;
        if (player.y < 0) player.y = 0;
        if (player.y > 550) player.y = 550;

        // Movimiento del enemigo
        enemy.x += enemy.speed_x;
        enemy.y += enemy.speed_y;

        if (enemy.x < 0 || enemy.x > 550) enemy.speed_x *= -1;
        if (enemy.y < 0 || enemy.y > 550) enemy.speed_y *= -1;

        drawSquare(player.x, player.y, player.size, 0.2f, 0.4f, 1.0f); // Azul
        drawSquare(enemy.x, enemy.y, enemy.size, 1.0f, 0.2f, 0.2f);    // Rojo

        // Colisión
        if (isCollision(player, enemy)) {
            std::cout << "¡Perdiste! Score: " << score << std::endl;
            player.x = 300; player.y = 300;
            enemy.x = rand() % 550; enemy.y = rand() % 550;
            score = 0;
        } else {
            score++;
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}