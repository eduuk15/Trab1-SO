#include <iostream>
#include <ncurses.h>
#include <unistd.h>  // Para sleep()
#include <vector>
#include <cstdlib>   // Para rand() e srand()
#include <ctime>     // Para o uso de tempo na geração aleatória de dinossauros
#include <algorithm> // Necessário para std::remove_if
#include <cmath>     // Para sqrt()
#include <pthread.h> // Para pthreads

class Game {
public:
    Game(int width, int height)
        : width(width), height(height), x(width - 25), y(height * 0.5), maxMissiles(6), missiles(6), missilesInDepot(6), truckMissiles(6), truckX(1), truckY(43), truckReached(false), shotsToKillDino(2), dinosaurGenerationTime(7000000), dinosaurGenerated(false) {
        initscr();
        noecho();
        cbreak();
        timeout(100);
        start_color();
        init_pair(1, COLOR_YELLOW, COLOR_BLACK); // Cor para o helicóptero
        init_pair(2, COLOR_RED, COLOR_BLACK);    // Cor para o míssil
        init_pair(3, COLOR_GREEN, COLOR_BLACK);  // Cor para o dinossauro
        init_pair(4, COLOR_BLUE, COLOR_BLACK);   // Cor para o caminhão
        init_pair(5, COLOR_MAGENTA, COLOR_BLACK); // Cor para o depósito

        srand(time(0));

        pthread_mutex_init(&mutexMissiles, nullptr);
        pthread_mutex_init(&mutexDinosaurs, nullptr);
        pthread_mutex_init(&mutexTruck, nullptr);
        pthread_mutex_init(&mutexDepot, nullptr);
    }

    ~Game() {
        endwin();
        pthread_mutex_destroy(&mutexMissiles);
        pthread_mutex_destroy(&mutexDinosaurs);
        pthread_mutex_destroy(&mutexTruck);
        pthread_mutex_destroy(&mutexDepot);
    }

     void setDifficulty(int difficulty) {
        switch (difficulty) {
            case 1: // Fácil
                shotsToKillDino = 1;
                missiles = 6;
                maxMissiles = 6;
                missilesInDepot = maxMissiles;
                truckMissiles = maxMissiles;
                dinosaurGenerationTime = 7000000;
                break;
            case 2: // Médio
                shotsToKillDino = 2;
                missiles = 5;
                maxMissiles = 5;
                missilesInDepot = maxMissiles;
                truckMissiles = maxMissiles;
                dinosaurGenerationTime = 5000000;
                break;
            case 3: // Difícil
                shotsToKillDino = 3;
                missiles = 4;
                maxMissiles = 4;
                missilesInDepot = maxMissiles;
                truckMissiles = maxMissiles;
                dinosaurGenerationTime = 3000000;
                break;
            default:
                std::cout << "Dificuldade inválida! Usando padrão: fácil." << std::endl;
                shotsToKillDino = 1;
                missiles = 6;
                maxMissiles = 6;
                dinosaurGenerationTime = 7000000;
        }
    }

    void drawHeader() {
        pthread_mutex_lock(&mutexMissiles);
        pthread_mutex_lock(&mutexTruck);
        pthread_mutex_lock(&mutexDinosaurs);
        pthread_mutex_lock(&mutexDepot);

        attron(A_BOLD);
        
        mvprintw(1, 30, "Mísseis no helicoptero: %d   ", missiles);
        
        mvprintw(2, 30, "Mísseis no caminhão: %d   ", truckMissiles);
        
        mvprintw(3, 30, "Mísseis no depósito: %d", missilesInDepot);

        mvprintw(4, 30, "Dinossauros restantes: %ld", dinosaurs.size());
        
        attroff(A_BOLD);

        pthread_mutex_unlock(&mutexMissiles);
        pthread_mutex_unlock(&mutexTruck);
        pthread_mutex_unlock(&mutexDinosaurs);
        pthread_mutex_unlock(&mutexDepot);
    }

    void drawHelicopter(int x, int y) {
        attron(COLOR_PAIR(1));
        mvprintw(y + 0, x, "--------+--------");
        mvprintw(y + 1, x, "  ___ /^^[___         _");
        mvprintw(y + 2, x, " /|^+----+   |#______//");
        mvprintw(y + 3, x, "( -+ |____|    _----+/");
        mvprintw(y + 4, x, " ==_________--'");
        mvprintw(y + 5, x, "  ~_|___|__");
    }

    void drawMissiles() {
        pthread_mutex_lock(&mutexMissiles);
        for (auto& missile : missilesList) {
            attron(COLOR_PAIR(2));
            mvprintw(missile.y, missile.x, "<<<<<<");
        }
        pthread_mutex_unlock(&mutexMissiles);
    }

    void drawDinosaurs() {
        pthread_mutex_lock(&mutexDinosaurs);
        for (auto& dino : dinosaurs) {
            attron(COLOR_PAIR(3));
            mvprintw(dino.y + 0, dino.x, "               __");
            mvprintw(dino.y + 1, dino.x, "              / _)");
            mvprintw(dino.y + 2, dino.x, "     _.----._/ /");
            mvprintw(dino.y + 3, dino.x, "    /         /");
            mvprintw(dino.y + 4, dino.x, " __/ (  | (  |");
            mvprintw(dino.y + 5, dino.x, "/__.-'|_|--|_|");
        }
        if (dinosaurs.size() >= maxDinosaurs) {
            mvprintw(5, 90, "Os dinossauros dominaram o mundo!");
            refresh();
            std::exit(0);
        } else if (dinosaurGenerated && dinosaurs.size() == 0) {
            mvprintw(5, 90, "Todos os dinossauros foram exterminados!");
            refresh();
            std::exit(0);
        }
        pthread_mutex_unlock(&mutexDinosaurs);
    }

    void drawTruck() {
        pthread_mutex_lock(&mutexTruck);
        attron(COLOR_PAIR(4));
        mvprintw(truckY, truckX, "===============\\");
        mvprintw(truckY + 1, truckX, "|----------||@  \\   ___");
        mvprintw(truckY + 2, truckX, "|____|_____|||_/_\\_|___|_");
        mvprintw(truckY + 3, truckX, "<|  ___\\    ||     | ____  |");
        mvprintw(truckY + 4, truckX, "<| /    |___||_____|/    | |");
        mvprintw(truckY + 5, truckX, "||/  O  |__________/  O  |_||");
        mvprintw(truckY + 6, truckX, "   \\___/            \\___/");
        pthread_mutex_unlock(&mutexTruck);
    }

    void drawDepot() {
        pthread_mutex_lock(&mutexDepot);
        attron(COLOR_PAIR(5));
        mvprintw(42, 160, "    _ _.-'`-._ _");
        mvprintw(43, 160, "   ;.'________'.;");
        mvprintw(44, 160, "________n.[____________].n_________");
        mvprintw(45, 160, "|\"\"_\"\"_\"\"_\"\"||==||==||==||\"\"_\"\"_\"\"_\"\"[");
        mvprintw(46, 160, "|\"\"\"\"\"\"\"\"\"\"||..||..||..||\"\"\"\"\"\"\"\"\"\"|");
        mvprintw(47, 160, "|LI LI LI LI||LI||LI||LI||LI LI LI LI|");
        mvprintw(48, 160, "|.. .. .. ..||..||..||..||.. .. .. ..|");
        mvprintw(49, 160, "|LI LI LI LI||LI||LI||LI||LI LI LI LI|");
        pthread_mutex_lock(&mutexDepot);
    }

    void updateMissiles() {
        pthread_mutex_lock(&mutexMissiles);
        for (auto& missile : missilesList) {
            missile.x -= 2;
        }
        missilesList.erase(std::remove_if(missilesList.begin(), missilesList.end(), [this](Missile& missile) {
            return missile.x < 0;
        }), missilesList.end());
        pthread_mutex_unlock(&mutexMissiles);
    }

    void fireMissile() {
        pthread_mutex_lock(&mutexMissiles);
        if (missiles > 0) {
            missilesList.push_back(Missile{x + 15, y + 4});
            missiles--;
        }
        pthread_mutex_unlock(&mutexMissiles);
    }

     void generateDinosaur() {
        if (dinosaurs.size() < 5) {
            int dinoX, dinoY;
            int attempts = 0;
            const int maxAttempts = 1000;

            do {
                dinoX = rand() % (width - 80) + 10;
                dinoY = rand() % (height - 20) + 5;

                pthread_mutex_lock(&mutexDinosaurs);
                bool tooCloseToHelicopter = isOverlapping(dinoX, dinoY, x, y, 30);
                bool tooCloseToOtherDinos = false;

                for (const auto& dino : dinosaurs) {
                    if (isOverlapping(dinoX, dinoY, dino.x, dino.y, 15)) {
                        tooCloseToOtherDinos = true;
                        break;
                    }
                }
                pthread_mutex_unlock(&mutexDinosaurs);

                if (!tooCloseToHelicopter && !tooCloseToOtherDinos) {
                    pthread_mutex_lock(&mutexDinosaurs);
                    dinosaurs.push_back(Dinosaur{dinoX, dinoY, 0});
                    dinosaurGenerated = true;
                    pthread_mutex_unlock(&mutexDinosaurs);
                    return;
                }
                attempts++;
            } while (attempts < maxAttempts);
        }
    }

    void checkHelicopterCollision() {
        pthread_mutex_lock(&mutexDinosaurs);
        std::vector<Dinosaur> remainingDinosaurs = dinosaurs;

        for (auto& dino : dinosaurs) {
            for (int i = 0; i < 5; ++i) {
                for (int j = 0; j < 5; ++j) {
                    if ((x + 23 >= dino.x && x <= dino.x + 18) && (y + 5 >= dino.y && y <= dino.y + 5)) {
                        mvprintw(5, 90, "O helicóptero explodiu!");
                        refresh();
                        std::exit(0);
                    }
                }
            }
        }
        pthread_mutex_unlock(&mutexDinosaurs);
    }

    void checkMissilesCollisions() {
        pthread_mutex_lock(&mutexMissiles);
        pthread_mutex_lock(&mutexDinosaurs);

        std::vector<Missile> remainingMissiles;
        std::vector<Dinosaur> remainingDinosaurs = dinosaurs;

        for (auto& missile : missilesList) {
            bool missileExploded = false;

            for (auto it = remainingDinosaurs.begin(); it != remainingDinosaurs.end(); ) {
                Dinosaur& dino = *it;

                if ((missile.y == dino.y || missile.y == dino.y + 1 || missile.y == dino.y + 2) && missile.x >= dino.x && missile.x <= dino.x + 18) {
                    dino.hits += 1;
                    if (dino.hits >= shotsToKillDino) {
                        it = remainingDinosaurs.erase(it);
                    }
                    missileExploded = true;
                    break;
                } else if ((missile.y >= dino.y + 2 && missile.y <= dino.y + 5) && missile.x >= dino.x && missile.x <= dino.x + 18) {
                    missileExploded = true;
                    break;
                } else {
                    ++it;
                }
            }

            if (!missileExploded) {
                remainingMissiles.push_back(missile);
            }
        }
        
        missilesList = std::move(remainingMissiles);
        dinosaurs = std::move(remainingDinosaurs);

        pthread_mutex_unlock(&mutexMissiles);
        pthread_mutex_unlock(&mutexDinosaurs);
    }

    void moveTruck() {
        pthread_mutex_lock(&mutexTruck);
        pthread_mutex_lock(&mutexDepot);

        if (truckReached) {
            if (truckX == 1) {
                truckReached = false;
                truckX += 2;
                truckMissiles = maxMissiles;
            } else {
                int spaceLeftInDepot = maxMissiles - missilesInDepot;
                int missilesToAdd = std::min(spaceLeftInDepot, truckMissiles);

                missilesInDepot += missilesToAdd;
                truckMissiles -= missilesToAdd;

                if (truckMissiles == 0) {
                    truckX -= 2;
                }
            }
        } else {
            truckX += 2;
            truckReached = truckX >= 160;
        }

        pthread_mutex_unlock(&mutexTruck);
        pthread_mutex_unlock(&mutexDepot);
    }

    void checkDepotCollision() {
        pthread_mutex_lock(&mutexDepot);
        pthread_mutex_lock(&mutexMissiles);
        if (x >= 150 && y == height - 15) {
            int spaceLeftInHelicopter = maxMissiles - missiles;
            int missilesToAdd = std::min(spaceLeftInHelicopter, missilesInDepot);

            missilesInDepot -= missilesToAdd;
            missiles += missilesToAdd;
        }
        pthread_mutex_unlock(&mutexDepot);
        pthread_mutex_unlock(&mutexMissiles);
    }

    static void* handleHelicopterCollision(void* arg) {
        Game* game = static_cast<Game*>(arg);
        while (true) {
            game->checkHelicopterCollision();
            usleep(30000);
        }
        return nullptr;
    }
    
    static void* handleMissilesCollisions(void* arg) {
        Game* game = static_cast<Game*>(arg);
        while (true) {
            game->checkMissilesCollisions();
            usleep(30000);
        }
        return nullptr;
    }

    static void* handleMissiles(void* arg) {
        Game* game = static_cast<Game*>(arg);
        while (true) {
            game->updateMissiles();
            usleep(30000);
        }
        return nullptr;
    }

    static void* handleDinosaur(void* arg) {
        Game* game = static_cast<Game*>(arg);
        while (true) {
            game->generateDinosaur();
            usleep(game->dinosaurGenerationTime);
        }
        return nullptr;
    }

    static void* handleTruck(void* arg) {
        Game* game = static_cast<Game*>(arg);
        while (true) {
            game->moveTruck();
            usleep(45000);
        }
        return nullptr;
    }
    
    static void* handleDepot(void* arg) {
        Game* game = static_cast<Game*>(arg);
        while (true) {
            game->checkDepotCollision();
            usleep(45000);
        }
        return nullptr;
    }



    void run() {
        pthread_t helicopterCollisionThread, missilesCollisionsThread, missileThread, dinosaurThread, truckThread, depotThread;
        pthread_create(&helicopterCollisionThread, nullptr, handleHelicopterCollision, this);
        pthread_create(&missilesCollisionsThread, nullptr, handleMissilesCollisions, this);
        pthread_create(&missileThread, nullptr, handleMissiles, this);
        pthread_create(&dinosaurThread, nullptr, handleDinosaur, this);
        pthread_create(&truckThread, nullptr, handleTruck, this);
        pthread_create(&depotThread, nullptr, handleDepot, this);

        while (true) {
            clear();
            drawHeader();
            drawHelicopter(x, y);
            drawMissiles();
            drawDinosaurs();
            drawTruck();
            drawDepot();

            int ch = getch();
            if (ch == 'q') {
                break;
            }
            if (ch == 'w' && y > 0) y--;
            if (ch == 's' && y < height - 15) y++;
            if (ch == 'a' && x > 0) x--;
            if (ch == 'd' && x < width) x++;
            if (ch == ' ') {
                fireMissile();
            }

            refresh();
        }

        pthread_join(missilesCollisionsThread, nullptr);
        pthread_join(missileThread, nullptr);
        pthread_join(dinosaurThread, nullptr);
        pthread_join(truckThread, nullptr);
    }

private:
    struct Missile {
        int x, y;
    };

    struct Dinosaur {
        int x, y, hits;
    };

    int width, height;
    int x, y;
    int maxMissiles;
    int missiles;
    int missilesInDepot;
    int truckMissiles;
    int truckX, truckY;
    bool truckReached;
    const int maxDinosaurs = 5;
    int shotsToKillDino;
    int dinosaurGenerationTime;
    bool dinosaurGenerated;


    pthread_mutex_t mutexMissiles;
    pthread_mutex_t mutexDinosaurs;
    pthread_mutex_t mutexTruck;
    pthread_mutex_t mutexDepot;

    std::vector<Missile> missilesList;
    std::vector<Dinosaur> dinosaurs;

    bool isOverlapping(int x1, int y1, int x2, int y2, int size) {
        return sqrt(pow(x1 - x2, 2) + pow(y1 - y2, 2)) <= size;
    }
};

int main() {
    int width = 200, height = 50;

    Game game(width, height);

    int difficulty;
    std::cout << "Escolha a dificuldade (1: Fácil, 2: Médio, 3: Difícil): ";
    std::cin >> difficulty;
    game.setDifficulty(difficulty);

    game.run();

    return 0;
}
