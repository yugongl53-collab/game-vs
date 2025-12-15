#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <random>
#include <algorithm>

// 按钮类
class Button
{
public:
    Button(float x, float y, float width, float height, const std::string& text)
    {
        shape.setPosition(x, y);
        shape.setSize(sf::Vector2f(width, height));
        shape.setFillColor(sf::Color(70, 70, 70));
        shape.setOutlineThickness(2);
        shape.setOutlineColor(sf::Color(200, 100, 50));
        
        buttonText.setString(text);
        buttonText.setCharacterSize(30);
        buttonText.setFillColor(sf::Color::White);
        
        this->x = x;
        this->y = y;
        this->width = width;
        this->height = height;
    }
    
    void setFont(const sf::Font& font)
    {
        buttonText.setFont(font);
        // 设置字体后重新计算文本位置
        updateTextPosition();
    }
    
    void updateTextPosition()
    {
        sf::FloatRect textBounds = buttonText.getLocalBounds();
        buttonText.setPosition(
            x + (width - textBounds.width) / 2 - textBounds.left,
            y + (height - textBounds.height) / 2 - textBounds.top
        );
    }
    
    bool isClicked(sf::Vector2f mousePos)
    {
        return shape.getGlobalBounds().contains(mousePos);
    }
    
    void draw(sf::RenderWindow& window)
    {
        window.draw(shape);
        window.draw(buttonText);
    }
    
    void setHovered(bool hovered)
    {
        if (hovered)
        {
            shape.setFillColor(sf::Color(100, 100, 100));
        }
        else
        {
            shape.setFillColor(sf::Color(70, 70, 70));
        }
    }

private:
    sf::RectangleShape shape;
    sf::Text buttonText;
    float x, y, width, height;
};

// 游戏状态枚举
enum GameState
{
    MENU,
    GAME,
    INFO,
    ABOUT
};

int main()
{
    sf::RenderWindow window(sf::VideoMode(1400, 1000), "Red Dead Redemption3");
    window.setFramerateLimit(60);
    
    // 加载背景纹理
    sf::Texture tcover;
    tcover.loadFromFile("D:\\game vs\\cover_bg.png");
    
    sf::Texture tgaming;
    tgaming.loadFromFile("D:\\game vs\\aiming-bg.png");
    
    sf::Texture tfali;
    tfali.loadFromFile("D:\\game vs\\fail.png");

    sf::Sprite scover;
    scover.setTexture(tcover);
    
    sf::Sprite sgaming;
    sgaming.setTexture(tgaming);

    sf::Sprite sfali;
    sfali.setTexture(tfali);

    // 敌人纹理与容器
    sf::Texture tenemy;
    sf::Image enemyImage;
    if (enemyImage.loadFromFile("D:\\game vs\\enemy.png"))
    {
        // 将偏蓝像素设为透明（蓝色通道显著高于红/绿）
        // 参数：最小蓝强度 minBlue 和蓝与最大(R,G)差值 thresh
        const int minBlue = 80;
        const int thresh = 40;
        sf::Vector2u sz = enemyImage.getSize();
        for (unsigned y = 0; y < sz.y; ++y)
        {
            for (unsigned x = 0; x < sz.x; ++x)
            {
                sf::Color c = enemyImage.getPixel(x, y);
                int b = c.b;
                int m = std::max<int>(c.r, c.g);
                if (b > minBlue && (b - m) > thresh)
                {
                    c.a = 0; // 设为透明
                    enemyImage.setPixel(x, y, c);
                }
            }
        }
        tenemy.loadFromImage(enemyImage);
    }
    else
    {
        std::cerr << "Failed to load enemy.png" << std::endl;
    }
    std::vector<sf::Sprite> enemies;
    // 随机数生成器用于敌人出现的位置
    std::mt19937 rng((unsigned)std::random_device{}());

    // 加载敌人开火时的替换纹理（enemyshoot.png），并做近蓝色透明处理
    sf::Texture tenemyShoot;
    bool tenemyShootLoaded = false;
    sf::Image enemyShootImage;
    if (enemyShootImage.loadFromFile("D:\\game vs\\enemyshoot.png"))
    {
        const int minBlue2 = 80;
        const int thresh2 = 40;
        sf::Vector2u sz2 = enemyShootImage.getSize();
        for (unsigned y = 0; y < sz2.y; ++y)
        {
            for (unsigned x = 0; x < sz2.x; ++x)
            {
                sf::Color c = enemyShootImage.getPixel(x, y);
                int b = c.b;
                int m = std::max<int>(c.r, c.g);
                if (b > minBlue2 && (b - m) > thresh2)
                {
                    c.a = 0;
                    enemyShootImage.setPixel(x, y, c);
                }
            }
        }
        tenemyShoot.loadFromImage(enemyShootImage);
        tenemyShootLoaded = true;
    }

    // 背景音乐
    sf::Music bgMusic;
    if (!bgMusic.openFromFile("D:\\game vs\\bgmusic.ogg"))
    {
        std::cerr << "Failed to load bgmusic.ogg" << std::endl;
    }
    else
    {
        bgMusic.setLoop(true);
    }

    // 射击音效（玩家与敌人共用），以及播放状态控制
    sf::SoundBuffer shootBuf;
    sf::Sound shootSound;
    bool shootLoaded = false;
    if (shootBuf.loadFromFile("D:\\game vs\\shoot.ogg"))
    {
        shootSound.setBuffer(shootBuf);
        shootLoaded = true;
    }
    else
    {
        std::cerr << "Failed to load shoot.ogg" << std::endl;
    }
    bool pendingEnemyShot = false; // 玩家射击后等待敌人回射
    bool enemyShooting = false;    // 敌人正在射击（期间禁止交互）

    sf::Vector2u windowSize = window.getSize();

    // 缩放cover背景
    sf::Vector2u textureSizeCover = tcover.getSize();
    float scaleXCover = static_cast<float>(windowSize.x) / textureSizeCover.x;
    float scaleYCover = static_cast<float>(windowSize.y) / textureSizeCover.y;
    scover.setScale(scaleXCover, scaleYCover);

    // 缩放gaming背景
    sf::Vector2u textureSizeGaming = tgaming.getSize();
    float scaleXGaming = static_cast<float>(windowSize.x) / textureSizeGaming.x;
    float scaleYGaming = static_cast<float>(windowSize.y) / textureSizeGaming.y;
    sgaming.setScale(scaleXGaming, scaleYGaming);

    // 缩放失败背景
    sf::Vector2u textureSizeFali = tfali.getSize();
    float scaleXFali = static_cast<float>(windowSize.x) / textureSizeFali.x;
    float scaleYFali = static_cast<float>(windowSize.y) / textureSizeFali.y;
    sfali.setScale(scaleXFali, scaleYFali);
    
    // 当前背景标记
        bool isGamingBackground = false;
        // 失败标记（计时到随机0.5-0.6s后触发）
        bool gameFailed = false;
        // 计时器：当切换到 aiming-bg 时开始计时，切回或失败时停止/重置
        sf::Clock aimingClock;
        bool aimingTimerRunning = false;
        // aiming 超时时间（每次进入图二时随机设定）
        float aimingTimeout = 0.5f;
        // 射击相关：只允许一次射击
        bool shotTaken = false;
        bool playerWon = false;
    
    // 加载字体
    sf::Font font;
    // 使用Windows系统字体
    if (!font.loadFromFile("C:\\Windows\\Fonts\\arial.ttf"))
    {
        std::cerr << "Failed to load font!" << std::endl;
    }
    
    // 游戏状态
    GameState currentState = MENU;
    
    // 创建菜单按钮
    Button startBtn(500, 350, 400, 80, "Start Game");
    Button infoBtn(500, 480, 400, 80, "Game Info");
    Button aboutBtn(500, 610, 400, 80, "About Us");
    Button exitBtn(500, 740, 400, 80, "Exit");
    
    // 返回菜单按钮
    Button backBtn(50, 50, 150, 50, "Back");
    // 重玩按钮（游戏结束后显示）
    Button playAgainBtn(550, 820, 300, 70, "Play Again");
    
    // 设置字体
    startBtn.setFont(font);
    infoBtn.setFont(font);
    aboutBtn.setFont(font);
    exitBtn.setFont(font);
    backBtn.setFont(font);
    playAgainBtn.setFont(font);
    
    // 标题文本
    sf::Text titleText;
    titleText.setFont(font);
    titleText.setString("Red Dead Redemption 3");
    titleText.setCharacterSize(60);
    titleText.setFillColor(sf::Color(200, 100, 50));
    
    // 居中标题
    sf::FloatRect titleBounds = titleText.getLocalBounds();
    titleText.setPosition(
        (1400 - titleBounds.width) / 2 - titleBounds.left,
        100 - titleBounds.top
    );
    
    // 信息文本
    sf::Text infoText;
    infoText.setFont(font);
    infoText.setString("Game Introduction\n\n"
                       "This is an immersive open-world action\n"
                       "adventure game set in the Wild West.\n\n"
                       "Features:\n"
                       "- Stunning graphics and environments\n"
                       "- Engaging storyline and characters\n"
                       "- Dynamic world with interactive NPCs\n"
                       "- Challenging missions and activities");
    infoText.setCharacterSize(24);
    infoText.setFillColor(sf::Color::White);
    infoText.setPosition(200, 150);
    
    // 关于我们文本
    sf::Text aboutText;
    aboutText.setFont(font);
    aboutText.setString("About Us\n\n"
                        "Developer Team: Game Studio X\n\n"
                        "Version: 3.0\n"
                        "Release Date: 2025\n\n"
                        "Powered by SFML\n"
                        "Built with passion for gaming");
    aboutText.setCharacterSize(24);
    aboutText.setFillColor(sf::Color::White);
    aboutText.setPosition(300, 200);

    // 胜利文本（初始化，但只在命中时显示）
    sf::Text winText;
    winText.setFont(font);
    winText.setCharacterSize(80);
    winText.setFillColor(sf::Color::Yellow);
    winText.setString("YOU WIN");
    sf::FloatRect winBounds = winText.getLocalBounds();
    winText.setPosition((1400 - winBounds.width) / 2 - winBounds.left, (1000 - winBounds.height) / 2 - winBounds.top);

    // 重置游戏的 lambda
    auto resetGame = [&]() {
        // 回到游戏界面并重置所有游戏相关状态
        currentState = GAME;
        isGamingBackground = false;
        gameFailed = false;
        playerWon = false;
        shotTaken = false;
        aimingTimerRunning = false;
        aimingTimeout = 0.0f;
        aimingClock.restart();
        enemies.clear();
        if (bgMusic.getStatus() != sf::Music::Playing)
            bgMusic.play();
        std::cout << "Game reset for replay." << std::endl;
    };
    
    while (window.isOpen())
    {
        sf::Event event;
        sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
        
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();
            
            // 右键点击：在游戏界面中直接切换背景（简化为无需长按/短按区分）
            if (event.type == sf::Event::MouseButtonReleased)
            {
                if (event.mouseButton.button == sf::Mouse::Right)
                {
                    if (currentState == GAME && !gameFailed && !playerWon && !pendingEnemyShot && !enemyShooting)
                    {
                        isGamingBackground = !isGamingBackground;
                        if (isGamingBackground)
                        {
                            // 切换到图二，随机设定计时超时并开始计时
                            std::uniform_real_distribution<float> distTimeout(0.5f, 0.6f);
                            aimingTimeout = distTimeout(rng);
                            aimingClock.restart();
                            aimingTimerRunning = true;
                            std::cout << "Aiming timeout set to: " << aimingTimeout << "s" << std::endl;

                            // 生成一个敌人，位置在窗口的水平中心线上随机横向位置
                            sf::Sprite enemy;
                            enemy.setTexture(tenemy);
                            sf::Vector2u eSize = tenemy.getSize();
                            float desiredWidth = 100.0f; // 期望敌人宽度，可调整
                            float scale = 1.0f;
                            if (eSize.x > 0)
                                scale = desiredWidth / static_cast<float>(eSize.x);
                            enemy.setScale(scale, scale);
                            // 随机横向位置，避开左右边缘各100像素
                            int minX = 100;
                            int maxX = static_cast<int>(windowSize.x) - 100 - static_cast<int>(eSize.x * scale);
                            if (maxX < minX) maxX = minX;
                            std::uniform_int_distribution<int> distX(minX, maxX);
                            float ex = static_cast<float>(distX(rng));
                            // 垂直位置固定在水平中心线（居中放置敌人）
                            float ey = static_cast<float>(windowSize.y) / 2.0f - (eSize.y * scale) / 2.0f;
                            enemy.setPosition(ex, ey);
                            enemies.push_back(enemy);
                        }
                        else
                        {
                            // 切回图一，停止并重置计时，并清除当前敌人
                            aimingTimerRunning = false;
                            aimingClock.restart();
                            aimingTimeout = 0.0f;
                            enemies.clear();
                        }
                        std::cout << "Background switched to: " << (isGamingBackground ? "aiming-bg" : "cover-bg") << std::endl;
                    }
                }
            }
            
            if (event.type == sf::Event::MouseButtonPressed)
            {
                if (event.mouseButton.button == sf::Mouse::Left)
                {
                    // 如果玩家已获胜或失败，优先检查重玩按钮，其次忽略其他左键操作
                    if (playerWon || gameFailed || pendingEnemyShot || enemyShooting)
                    {
                        // 游戏结束或敌人正在回击期间，只允许在结束时点击重玩
                        if ((playerWon || gameFailed) && playAgainBtn.isClicked(mousePos))
                        {
                            resetGame();
                        }
                        // 无论是否点击重玩，跳过其他左键交互
                        continue;
                    }

                    if (currentState == MENU)
                    {
                        if (startBtn.isClicked(mousePos))
                        {
                            currentState = GAME;
                            std::cout << "Starting Game..." << std::endl;
                            if (bgMusic.getStatus() != sf::Music::Playing)
                                bgMusic.play();
                        }
                        else if (infoBtn.isClicked(mousePos))
                        {
                            currentState = INFO;
                            std::cout << "Showing Game Info..." << std::endl;
                        }
                        else if (aboutBtn.isClicked(mousePos))
                        {
                            currentState = ABOUT;
                            std::cout << "Showing About Us..." << std::endl;
                        }
                        else if (exitBtn.isClicked(mousePos))
                        {
                            window.close();
                        }
                    }
                    else if (currentState == GAME)
                    {
                        // 如果处于图二并且尚未射击，左键为射击
                        if (isGamingBackground && !shotTaken && !gameFailed && !playerWon)
                        {
                            shotTaken = true; // 只允许一次射击
                            bool hit = false;
                            // 检测是否击中任一敌人
                            for (auto &e : enemies)
                            {
                                if (e.getGlobalBounds().contains(mousePos))
                                {
                                    hit = true;
                                    break;
                                }
                            }
                            if (hit)
                            {
                                playerWon = true;
                                aimingTimerRunning = false;
                                enemies.clear(); // 隐藏敵人
                                if (bgMusic.getStatus() == sf::Music::Playing) bgMusic.stop();
                                // 播放射击音效（命中也发声）
                                if (shootLoaded)
                                    shootSound.play();
                                std::cout << "Player hit an enemy: YOU WIN" << std::endl;
                            }
                            else
                            {
                                // 玩家发射音效（如果可用）并等待敌人回射
                                if (shootLoaded)
                                {
                                    shootSound.play();
                                }
                                // 标记等待敌人回射
                                pendingEnemyShot = true;
                                aimingTimerRunning = false; // 停掉计时（敌人回射期间不再计时）
                                std::cout << "Player missed: enemy will shoot back" << std::endl;
                            }
                        }

                        // 返回主菜单的条件：非目标模式或已射击或失败
                        if (!isGamingBackground || shotTaken || gameFailed)
                        {
                            if (backBtn.isClicked(mousePos))
                            {
                                currentState = MENU;
                                std::cout << "Back to Menu..." << std::endl;
                            }
                        }
                    }
                    else if (currentState == INFO || currentState == ABOUT)
                    {
                        if (backBtn.isClicked(mousePos))
                        {
                            currentState = MENU;
                            std::cout << "Back to Menu..." << std::endl;
                        }
                    }
                }
            }
        }
        
        // 检查鼠标悬停
        if (currentState == MENU)
        {
            startBtn.setHovered(startBtn.isClicked(mousePos));
            infoBtn.setHovered(infoBtn.isClicked(mousePos));
            aboutBtn.setHovered(aboutBtn.isClicked(mousePos));
            exitBtn.setHovered(exitBtn.isClicked(mousePos));
        }
        else
        {
            backBtn.setHovered(backBtn.isClicked(mousePos));
        }
        
        window.clear();
        
        // 计时检查：如果正在计时且到达随机超时，触发失败并切换到 fail 背景
        if (aimingTimerRunning && !gameFailed && aimingTimeout > 0.0f)
        {
            if (aimingClock.getElapsedTime().asSeconds() >= aimingTimeout)
            {
                gameFailed = true;
                aimingTimerRunning = false;
                isGamingBackground = false; // 停止 aiming 背景
                if (bgMusic.getStatus() == sf::Music::Playing) bgMusic.stop();
                std::cout << "Timer reached " << aimingTimeout << "s -> Game Failed, switching to fail.png" << std::endl;
            }
        }

        // 处理射击音效的顺序：玩家未命中后先播放玩家射击（已触发），
        // 待其播放完成则触发敌人回射；敌人回射期间禁止玩家操作，回射结束判定为失败。
        if (pendingEnemyShot)
        {
            // 等待玩家射击声播完再让敌人回射
            if (!shootLoaded || shootSound.getStatus() != sf::Sound::Playing)
            {
                // 在敌人回射前替换敌人贴图为开火贴图（保持位置和缩放）
                if (tenemyShootLoaded)
                {
                    for (auto &e : enemies)
                    {
                        sf::Vector2f prevScale = e.getScale();
                        sf::Vector2f prevPos = e.getPosition();
                        e.setTexture(tenemyShoot);
                        e.setScale(prevScale);
                        e.setPosition(prevPos);
                    }
                }
                // 播放敌人回射音
                if (shootLoaded)
                    shootSound.play();
                pendingEnemyShot = false;
                enemyShooting = true;
                std::cout << "Enemy is shooting..." << std::endl;
            }
        }

        if (enemyShooting)
        {
            // 等待敌人射击音播放完成
            if (!shootLoaded || shootSound.getStatus() != sf::Sound::Playing)
            {
                enemyShooting = false;
                // 敌人回射完毕，判定失败
                gameFailed = true;
                isGamingBackground = false;
                if (bgMusic.getStatus() == sf::Music::Playing) bgMusic.stop();
                std::cout << "Enemy finished shooting -> GAME FAILED" << std::endl;
            }
        }

        // 绘制背景与游戏状态
        if (gameFailed)
        {
            window.draw(sfali);
            // 显示重玩按钮
            playAgainBtn.draw(window);
        }
        else if (playerWon)
        {
            // 玩家胜利时：显示图二背景，隐藏敌人，显示胜利文字（后面统一绘制）
            window.draw(sgaming);
            // 显示重玩按钮
            playAgainBtn.draw(window);
        }
        else if (currentState == GAME)
        {
            // 游戏界面显示对应的背景
            if (isGamingBackground)
            {
                window.draw(sgaming);
            }
            else
            {
                window.draw(scover);
            }

            // 绘制敌人（如果有且未胜利）
            for (auto &e : enemies)
            {
                window.draw(e);
            }
            // 如果此时游戏失败（虽然进入本分支说明未失败），不显示重玩；
            // playAgain 在失败或胜利分支中绘制
        }
        else
        {
            // 其他界面显示cover背景
            window.draw(scover);
        }
        
        if (currentState == MENU)
        {
            // 绘制菜单
            window.draw(titleText);
            startBtn.draw(window);
            infoBtn.draw(window);
            aboutBtn.draw(window);
            exitBtn.draw(window);
        }
        else if (currentState == GAME)
        {
            // 游戏界面 - 直接显示背景
        }
        else if (currentState == INFO)
        {
            // 绘制游戏介绍
            window.draw(infoText);
            backBtn.draw(window);
        }
        else if (currentState == ABOUT)
        {
            // 绘制关于我们
            window.draw(aboutText);
            backBtn.draw(window);
        }
        // 如果玩家胜利，绘制胜利文本（仅显示一次）
        if (playerWon)
        {
            window.draw(winText);
        }
        
        window.display();
    }
    
    return 0;
}