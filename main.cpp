#include <iostream>
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/System.hpp>
#include <SFML/Audio.hpp>
#include <chrono>
#include <fstream> 
#include <vector>
#include <numeric>
#include <string>
using namespace std::chrono;

#define MAX_CHARACTERS  10

class Board;
class boardMap {
    private:
        int numCol, numRow, numMine;
        std::vector<std::vector<int>> mine;
        std::vector<std::vector<int>> number;
    public:
        boardMap(int numCol, int numRow, int numMine): numCol(numCol), numRow(numRow), numMine(numMine) {
            mine = std::vector<std::vector<int>> (numRow, std::vector<int> (numCol, 0));
            number = std::vector<std::vector<int>> (numRow, std::vector<int> (numCol, 0));
        }

        void RandomMineMap() {
            std::vector<int> permutation(numCol * numRow);
            std::iota(permutation.begin(), permutation.end(), 0);
            std::random_shuffle(permutation.begin(), permutation.end());
            // return;
            for (int i = 0; i < numRow; i++)
                for (int j = 0; j < numCol; j++)
                    mine[i][j] = number[i][j] = 0;
                
            for (int i = 0; i < numMine; i++) {
                int row = permutation[i] / numCol;
                int col = permutation[i] % numCol;
                mine[row][col] = 1;
            }
            for (int i = 0; i < numRow; i++)
            for (int j = 0; j < numCol; j++)
                if (mine[i][j])
                    number[i][j] = -1;
                else {
                    for (int x = -1; x <= 1; x++)
                    for (int y = -1; y <= 1; y++)
                        if (i + x >= 0 && i + x < numRow && j + y >= 0 && j + y < numCol) 
                            number[i][j] += mine[i + x][j + y];
                }
        }
        friend class Board;
};

class Record {
    public:
        std::string time, name;
        Record(std::string s) {
            auto it = s.find(",");
            time = s.substr(0, it);
            name = s.substr(it + 1);
        }

        Record(std::string t, std::string n):time(t), name(n) {

        }

        bool operator < (Record other) {
            return this->time < other.time;
        }

        bool operator == (Record other) {
            return this->time == other.time && this->name == other.name;
        }

        std::string getTime() {
            return time;
        }

        std::string getName() {
            return name;
        }
};

class Leaderboard {
    private:
        std::vector<Record> records;

    public:
        Leaderboard() {
            std::ifstream os;
            os.open("leaderboard.txt");
            std::string s;
            while(std::getline(os, s))
                records.push_back(Record(s));
            os.close();
            sort(records.begin(), records.end());
        }

        std::string convertTime(int time) {
            int minutes = std::min(99,time / 60);
            int seconds = time % 60;
            std::string t;
            if (minutes < 10)
                t = "0" + std::to_string(minutes);
            else 
                t = std::to_string(minutes);
            t.push_back(':');
            if (seconds < 10)
                t += "0" + std::to_string(seconds);
            else 
                t += std::to_string(seconds);
            return t;
        }
        void addRecord(int time, std::string name) {
            std::string t = convertTime(time);
            records.push_back(Record(t, name));
            sort(records.begin(), records.end());
        }

        ~Leaderboard() {
            std::ofstream os;
            os.open("leaderboard.txt");
            for (int i = 0; i < 5; i++)
                os << records[i].getTime() << ',' << records[i].getName() << '\n';
            os.close();
        }

        int getRecordSize() {
            return records.size();
        }

        Record getRecord(int id) {
            return records[id];
        }

};

int LeaderboardWindowProcess(int width, int height, int, std::string);
class Board {
    private:
        const int NOTOPEN = 0;
        const int OPENED = 1;
        const int FLAGED = 2;
        const int DEBUG = 3;
        boardMap state;
        std::string playerName;
        std::vector<sf::Sprite> cells;
        std::vector<sf::Texture> cellTextures;

        std::vector<sf::Sprite> icons;
        std::vector<sf::Texture> iconTextures;

        std::vector<sf::Sprite> flagCounters;
        std::vector<sf::IntRect> flagCounterTextures;

        std::vector<sf::Sprite> timeCounters;
        std::vector<sf::IntRect> timeCounterTextures;
        
        std::vector<int> gameState;

        sf::RectangleShape faceButton, debugButton, playPauseButton, leaderboardButton;
        sf::Texture faceTexture, debugTexture, playPauseTexture, leaderboardTexture;
        
        sf::Texture digitsTexture, tileHiddenTexture, tileRevealedTexture;

        time_point<high_resolution_clock> current, lastPlay; 
        duration<double> totalTime; 

        int timeCounter, flagCounter, tileCounter;
        int isWin, isPause, isDebugging, isFirstAction, isLeaderboardAfterWin;

    public:
        Board(boardMap state, std::string playerName) : state(state), playerName(playerName) {
            digitsTexture.loadFromFile("./images/digits.png");
            tileHiddenTexture.loadFromFile("./images/tile_hidden.png");
            tileRevealedTexture.loadFromFile("./images/tile_revealed.png");
        }

        void init() {
            int boardSize = state.numCol * state.numRow;
            cells.resize(boardSize);
            cellTextures.resize(boardSize);
            icons.resize(boardSize);
            iconTextures.resize(boardSize);
            gameState = std::vector<int> (boardSize, NOTOPEN);
            flagCounters.resize(3);
            flagCounterTextures.resize(3);
            timeCounters.resize(4);
            timeCounterTextures.resize(4);



            for (int i = 0; i < state.numRow; i++) {
                for (int j = 0; j < state.numCol; j++) {
                    int yPos = i * 32; 
                    int xPos = j * 32;
                    int id = i * state.numCol + j;

                    cellTextures[id].loadFromFile("./images/tile_hidden.png");
                    cells[id].setTexture(cellTextures[id]);
                    cells[id].setPosition(xPos, yPos);

                    iconTextures[id].loadFromFile("./images/tile_revealed.png");
                    icons[id].setTexture(iconTextures[id]);
                    icons[id].setPosition(xPos, yPos);
                }
            }

            int width = state.numCol;
            int height = state.numRow;

            for (int i = 0; i < 3; i++) {
                flagCounterTextures[i].height = 32;
                flagCounterTextures[i].width = 21;
                flagCounterTextures[i].top = 0;
                flagCounters[i].setTextureRect(flagCounterTextures[i]);
                flagCounters[i].setPosition(sf::Vector2f(33 + 21 * i, 32 * (height+0.5f)+16));
                flagCounters[i].setTexture(digitsTexture);
            }

            for (int i = 0; i < 2; i++) {
                timeCounterTextures[i].height = 32;
                timeCounterTextures[i].width = 21;
                timeCounterTextures[i].top = 0;
                timeCounters[i].setTextureRect(timeCounterTextures[i]);
                timeCounters[i].setPosition(sf::Vector2f((width * 32) - 97 + 21 * i, 32 * (height + 0.5f) + 16));
                timeCounters[i].setTexture(digitsTexture);
            }

            for (int i = 2; i < 4; i++) {
                timeCounterTextures[i].height = 32;
                timeCounterTextures[i].width = 21;
                timeCounters[i].setTextureRect(timeCounterTextures[i]);
                timeCounters[i].setPosition(sf::Vector2f((width * 32) - 54 + 21 * (i - 2), 32 * (height + 0.5f) + 16));
                timeCounters[i].setTexture(digitsTexture);
            }

            faceTexture.loadFromFile("./images/face_happy.png");
            faceButton.setSize(sf::Vector2f(64, 64));
            faceButton.setTexture(&faceTexture);
            faceButton.setPosition(sf::Vector2f(((width / 2.0) * 32) - 32, 32 *(height+0.5f)));

            debugTexture.loadFromFile("./images/debug.png");
            debugButton.setSize(sf::Vector2f(64, 64));
            debugButton.setPosition(sf::Vector2f((width * 32) - 304, 32 * (height+0.5f)));
            debugButton.setTexture(&debugTexture);

            playPauseTexture.loadFromFile("./images/pause.png");
            playPauseButton.setSize(sf::Vector2f(64, 64));
            playPauseButton.setPosition(sf::Vector2f((width * 32) - 240, 32 * (height+0.5f)));
            playPauseButton.setTexture(&playPauseTexture);

            leaderboardTexture.loadFromFile("./images/leaderboard.png");
            leaderboardButton.setSize(sf::Vector2f(64, 64));
            leaderboardButton.setPosition(sf::Vector2f((width * 32) - 176, 32 * (height+0.5f)));
            leaderboardButton.setTexture(&leaderboardTexture);   

            flagCounter = state.numMine;
            tileCounter = state.numRow * state.numCol - state.numMine;
            timeCounter = 0;
            isWin = 0;
            isPause = 0;
            isDebugging = 0;
            isFirstAction = 1;
            isLeaderboardAfterWin = 0;
            totalTime = duration_cast<duration<double>>(
                high_resolution_clock::now() - high_resolution_clock::now());
        }

        void debug(int xPos, int yPos) {
            if (isWin) return;

            if (isFirstAction) 
                lastPlay = high_resolution_clock::now();
            
            if (debugButton.getGlobalBounds().contains(sf::Vector2f(xPos, yPos))) {
                isDebugging = 1 - isDebugging;
                for (int i = 0; i < gameState.size(); i++) 
                if (state.mine[i / state.numCol][i % state.numCol]) {
                    if (isDebugging) {
                        if (gameState[i] == FLAGED)
                            flagCounter++;
                        gameState[i] = DEBUG;
                        iconTextures[i].loadFromFile("./images/mine.png");
                    } else 
                        if (gameState[i] == DEBUG)
                            gameState[i] = NOTOPEN;                    
                }
            }

        }

        void face(int xPos, int yPos) {
            if (faceButton.getGlobalBounds().contains(sf::Vector2f(xPos, yPos))) {
                state.RandomMineMap();
                init();
            }
        }

        void pause(int xPos, int yPos, int force = 0) {
            if (playPauseButton.getGlobalBounds().contains(sf::Vector2f(xPos, yPos)) && isWin == 0) {
                if (isPause) {
                    playPauseTexture.loadFromFile("./images/pause.png");
                    isPause = 0;
                    lastPlay = high_resolution_clock::now();
                } else {
                    playPauseTexture.loadFromFile("./images/play.png");
                    isPause = 1;
                    current = high_resolution_clock::now();
                    totalTime += duration_cast<duration<double>>(current - lastPlay);
                }
            }
            if (force == 1 && isPause == 0) {
                playPauseTexture.loadFromFile("./images/play.png");
                isPause = 2;
                current = high_resolution_clock::now();
                totalTime += duration_cast<duration<double>>(current - lastPlay);
            } else if (force == 1 && isPause == 2) {
                playPauseTexture.loadFromFile("./images/pause.png");
                isPause = 0;
                lastPlay = high_resolution_clock::now();
            }
        }

        bool leaderboard(int xPos, int yPos) {
            if (leaderboardButton.getGlobalBounds().contains(sf::Vector2f(xPos, yPos))) {
                return true;
            }
            else
                return false;
        }

        void drawFlag(sf::RenderWindow &window) {
            int digits[3];
            // std::cerr << flagCounter << "\n";
            if (flagCounter < 0) 
                digits[0] = 10;
            else 
                digits[0] = flagCounter / 100;
            int temp = (flagCounter < 0) ? std::min(abs(flagCounter), 99) : std::min(flagCounter, 999);

            digits[1] = temp / 10 % 10;
            digits[2] = temp % 10;

            for (int i = 0; i < 3; i++) {
                flagCounterTextures[i].left = 21 * digits[i];
                flagCounters[i].setTextureRect(flagCounterTextures[i]);
                window.draw(flagCounters[i]);
            }
        }

        void drawTime(sf::RenderWindow &window) {
            int digits[4];
            if (isPause == 0 && !isFirstAction && isWin == 0) {
                current = high_resolution_clock::now();
                duration<double> moreTime = duration_cast<duration<double>>(current - lastPlay);
                timeCounter = totalTime.count() + moreTime.count();
            } else
                timeCounter = totalTime.count();
            int temp = std::min(timeCounter, 5999);
            // temp = 599;
            digits[0] = temp / 600;
            digits[1] = temp / 60 % 10;
            digits[2] = temp % 60 / 10;
            digits[3] = temp % 60 % 10;
            for (int i = 0; i < 4; i++) {
                timeCounterTextures[i].left = 21 * digits[i];
                timeCounters[i].setTextureRect(timeCounterTextures[i]);
                window.draw(timeCounters[i]);
            }
            return;
        }

        void PrintBoard(sf::RenderWindow &window) {
            window.clear(sf::Color::White);
            if (isWin == 1) {
                flagCounter = 0;
                for (int i = 0; i < cells.size(); i++)
                    if (gameState[i] == NOTOPEN || gameState[i] == DEBUG) {
                        iconTextures[i].loadFromFile("./images/flag.png");
                        gameState[i] = FLAGED;
                    }
                faceTexture.loadFromFile("./images/face_win.png");
            } else if (isWin == -1) {
                for (int i = 0; i < cells.size(); i++) {
                    int r = i / state.numCol;
                    int c = i % state.numCol;
                    if (state.mine[r][c]) {
                        gameState[i] = OPENED;
                        cellTextures[i].loadFromFile("./images/tile_revealed.png");
                        iconTextures[i].loadFromFile("./images/mine.png");
                    }
                }
                faceTexture.loadFromFile("./images/face_lose.png");
            }
            if (isPause) 
                for (int i = 0; i < cells.size(); i++)
                    cells[i].setTexture(tileRevealedTexture);
            else 
                for (int i = 0; i < cells.size(); i++)
                    cells[i].setTexture(cellTextures[i]);
            
            for (int i = 0; i < cells.size(); i++) {
                window.draw(cells[i]);
                if (gameState[i] != NOTOPEN && isPause == 0) {
                    window.draw(icons[i]);
                }
            }
            window.draw(faceButton);
            window.draw(debugButton);
            window.draw(playPauseButton);
            window.draw(leaderboardButton);
            drawFlag(window);
            drawTime(window);
            window.display();
            if (isWin == 1 && isLeaderboardAfterWin == 0) {
                isLeaderboardAfterWin = 1;
                LeaderboardWindowProcess(window.getSize().x, window.getSize().y, timeCounter, playerName);;
            }
            return;
        }

        void setFlag(int xPos, int yPos) {
            if (isWin || isPause) return;
            if (xPos < 0 || yPos < 0)
                return;
            if (xPos >= 32 * state.numCol || yPos >= 32 * state.numRow)
                return;

            if (isFirstAction) {
                lastPlay = high_resolution_clock::now();
                isFirstAction = 0;
            }
            

            int i = yPos / 32;
            int j = xPos / 32;
            int id = i * state.numCol + j;
            if (gameState[id] == OPENED) 
                return;
            if (gameState[id] == NOTOPEN || gameState[id] == DEBUG) {
                gameState[id] = FLAGED;
                iconTextures[id].loadFromFile("./images/flag.png");
                --flagCounter;
            } else {
                if (state.mine[i][j] && isDebugging) {
                    gameState[id] = DEBUG;
                    iconTextures[id].loadFromFile("./images/mine.png");
                }
                else {
                    gameState[id] = NOTOPEN;
                    iconTextures[id].loadFromFile("./images/tile_revealed.png");
                }
                ++flagCounter;
            }

        }

        void openCell(int xPos, int yPos) {
            if (isWin || isPause) return;
            if (xPos < 0 || yPos < 0)
                return;
            if (xPos >= 32 * state.numCol || yPos >= 32 * state.numRow)
                return;

            if (isFirstAction) {
                lastPlay = high_resolution_clock::now();
                isFirstAction = 0;
            }

            int i = yPos / 32;
            int j = xPos / 32;
            int id = i * state.numCol + j;

            if (gameState[id] == OPENED || gameState[id] == FLAGED)
                return;

            gameState[id] = OPENED;
            cellTextures[id].loadFromFile("./images/tile_revealed.png");   
            if (state.mine[i][j]) {
                iconTextures[id].loadFromFile("./images/mine.png");
                isWin =  -1;
                tileCounter++;
                current = high_resolution_clock::now();
                duration<double> moreTime = duration_cast<duration<double>>(current - lastPlay);
                totalTime += moreTime;
            }
            else if (state.number[i][j] == 0) {
                std::vector<std::pair<int, int>> st;
                st.push_back({i, j});
                while (st.size()) {
                    int r = st.back().first;
                    int c = st.back().second;
                    st.pop_back();
                    for (int dr = -1; dr <= 1; dr++)
                    for (int dc = -1; dc <= 1; dc++) 
                    if (dr * dr + dc * dc != 0) {
                        int nr = r + dr;
                        int nc = c + dc;
                        if (nr < 0 || nr >= state.numRow) continue;
                        if (nc < 0 || nc >= state.numCol) continue;
                        int id = nr * state.numCol + nc;
                        if (gameState[id] != NOTOPEN) continue;
                        gameState[id] = OPENED;
                        tileCounter--;
                        if (state.number[nr][nc] == 0)
                            st.push_back({nr, nc});
                        else {
                            std::string file = "./images/number_" + std::to_string(state.number[nr][nc]) + ".png";
                            iconTextures[id].loadFromFile(file.c_str());
                            cellTextures[id].loadFromFile("./images/tile_revealed.png");
                        }
                    }
                }
            }
            else {
                std::string file = "./images/number_" + std::to_string(state.number[i][j]) + ".png";
                iconTextures[id].loadFromFile(file.c_str());
            }
            tileCounter--;
            if (tileCounter == 0) {
                isWin = 1;
                current = high_resolution_clock::now();
                duration<double> moreTime = duration_cast<duration<double>>(current - lastPlay);
                totalTime += moreTime;
            }
        }

};


void LoadBoardConfig(int &numCol, int &numRow, int &numMine) {
    std::ifstream os;
    os.open("board_config.cfg");
    os >> numCol >> numRow >> numMine;
    // std::cerr << numRow << numCol << numMine;
    os.close();
    return;
}

int LeaderboardWindowProcess(int width, int height, int time = -10, std::string playerName = "") {
    width /= 2;
    height /= 2;
    sf::RenderWindow window(sf::VideoMode(width, height), "Leaderboard Window");
    window.setFramerateLimit(60);
    sf::Font font;
    if (!font.loadFromFile("font.ttf")) {
        std::cerr << "Error: font.ttf cannot be loaded" << std::endl;
        return 1;
    }
    sf::Text title;
    title.setStyle(sf::Text::Bold | sf::Text::Underlined);
    title.setFillColor(sf::Color::White);
    title.setCharacterSize(20);
    title.setFont(font);
    title.setString("LEADERBOARD");
    title.setPosition(sf::Vector2f(width / 2.0f - title.getGlobalBounds().width / 2, height / 2.0f - 120));

    Leaderboard leaderboard; 
    if (time >= 0)
        leaderboard.addRecord(time, playerName);

    int notPrinted = (time >= 0);
    std::string print;
    for (int i = 0; i < 5; i++) {
        if (leaderboard.getRecordSize() <= i) break;
        Record item = leaderboard.getRecord(i);
        print += std::to_string(i+1) + ".\t" + item.getTime() + "\t" + item.getName();
        if (item == Record(leaderboard.convertTime(time), playerName) && notPrinted) {
            print += "*";
            notPrinted = 0;
        }
        print += "\n\n";
    }
    sf::Text content;
    content.setStyle(sf::Text::Bold);
    content.setFillColor(sf::Color::White);
    content.setCharacterSize(18);
    content.setFont(font);
    content.setString(print);
    content.setPosition(sf::Vector2f(width / 2.0f - content.getGlobalBounds().width / 2, height / 2.0f + 120 - content.getGlobalBounds().height));


    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
                return 0;
            }
        }
        window.clear(sf::Color::Blue);
        window.draw(title);
        window.draw(content);
        window.display();
    }
    return 0;
}

int GameWindowProcess(std::string playerName) {
    int numRow, numCol, numMine;
    LoadBoardConfig(numCol, numRow, numMine);
    int width = numCol * 32;
    int heigh = numRow * 32 + 100;
    sf::RenderWindow window(sf::VideoMode(width, heigh), "Game Window");
    window.setFramerateLimit(60);
    sf::Font font;
    if (!font.loadFromFile("font.ttf")) {
        std::cerr << "Error: font.ttf cannot be loaded" << std::endl;
        return 1;
    }
    
    boardMap state(numCol, numRow, numMine);
    state.RandomMineMap();
    Board game(state, playerName);
    game.init();
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
                return 0;
            }
            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                // std::cerr << "Left click detected\n";
                sf::Vector2i mousePosition = sf::Mouse::getPosition(window);

                // Print the mouse position to the console
                // std::cout << "Left click at position: " << mousePosition.x << ", " << mousePosition.y << std::endl;
                game.openCell(mousePosition.x, mousePosition.y);
                game.debug(mousePosition.x, mousePosition.y);
                game.face(mousePosition.x, mousePosition.y);
                game.pause(mousePosition.x, mousePosition.y);
                if (game.leaderboard(mousePosition.x, mousePosition.y)) {
                    game.pause(-1, -1, 1);
                    game.PrintBoard(window);
                    LeaderboardWindowProcess(window.getSize().x, window.getSize().y);
                    game.pause(-1, -1, 1);
                }
            }
            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Right) {
                // std::cerr << "Right click detected\n";
                sf::Vector2i mousePosition = sf::Mouse::getPosition(window);
                // std::cout << "Left click at position: " << mousePosition.x << ", " << mousePosition.y << std::endl;
                game.setFlag(mousePosition.x, mousePosition.y);

            }
        }
        game.PrintBoard(window);
    }
    return 0;
}

int main() {

    sf::RenderWindow window(sf::VideoMode(800, 600), "Welcome Window");
    window.setFramerateLimit(60);

    sf::Font font;
    if (!font.loadFromFile("font.ttf")) {
        std::cerr << "Error: font.ttf cannot be loaded" << std::endl;
        return 1;
    }

    sf::Text welcomeText;
    welcomeText.setFont(font);
    welcomeText.setCharacterSize(24);
    welcomeText.setStyle(sf::Text::Bold | sf::Text::Underlined);
    welcomeText.setString("WELCOME TO MINESWEEPER!");
    welcomeText.setFillColor(sf::Color::White);
    welcomeText.setPosition(window.getSize().x / 2 - welcomeText.getGlobalBounds().width / 2,
                            window.getSize().y / 2 - 150);

    sf::Text instructions;
    instructions.setFont(font);
    instructions.setCharacterSize(20);
    instructions.setStyle(sf::Text::Bold);
    instructions.setString("Enter your name:");
    instructions.setFillColor(sf::Color::White);
    instructions.setPosition(window.getSize().x / 2 - instructions.getGlobalBounds().width / 2,
                             window.getSize().y / 2 - 75);

    std::string playerName;
    sf::Text playerNameText;
    playerNameText.setFont(font);
    playerNameText.setCharacterSize(18);
    playerNameText.setStyle(sf::Text::Bold);
    playerNameText.setString(playerName);
    playerNameText.setFillColor(sf::Color::Yellow);
    playerNameText.setPosition(window.getSize().x / 2 - playerNameText.getGlobalBounds().width / 2,
                               window.getSize().y / 2 - 45);

    sf::Text cursor;
    cursor.setFont(font);
    cursor.setCharacterSize(18);
    cursor.setString("|");
    cursor.setFillColor(sf::Color::Yellow);
    cursor.setPosition(playerNameText.findCharacterPos(playerName.size()).x,
                       playerNameText.getPosition().y + playerNameText.getGlobalBounds().height + 10);

    sf::RectangleShape background;
    background.setSize(sf::Vector2f(window.getSize().x, window.getSize().y));
    background.setFillColor(sf::Color::Blue);

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::TextEntered) {
                if (std::isalpha(event.text.unicode) && playerName.size() < MAX_CHARACTERS) {
                    playerName += static_cast<char>(event.text.unicode);
                }
            } else if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Backspace && !playerName.empty()) {
                    // Handle backspace
                    playerName.pop_back();
                } else if (event.key.code == sf::Keyboard::Enter && playerName.size() > 0) {
                    window.close();
                }
            } else if (event.type == sf::Event::Closed) {
                window.close();
                return 0;
            }
            playerName[0] = toupper(playerName[0]);
            for (size_t i = 1; i < playerName.size(); ++i) {
                playerName[i] = tolower(playerName[i]);
            }
        }

        playerNameText.setString(playerName);

        cursor.setPosition(playerNameText.getPosition().x + playerNameText.getGlobalBounds().width, playerNameText.getPosition().y);

        window.clear(sf::Color::Blue);
        window.draw(welcomeText);
        window.draw(instructions);
        window.draw(playerNameText);
        window.draw(cursor);
        window.display();
    }

    // Proceed to game window
    // ...
    GameWindowProcess(playerName);
    return 0;
}