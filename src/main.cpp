#include <glad/gl.h> // Must be first!
#include <SFML/Graphics.hpp>
#include <thread>
#include <atomic>
#include <iostream>
#include <memory>
#include <optional>
#include <thread>
#include <string>
#include "triangle.h"
#include "glyph.h"

std::vector<Path> paths;
float scaleMultiplier = 0.9;
float viewX = 0.f;


void drawGlyph(sf::RenderWindow& window,
               const std::vector<Path>& paths)
{
    for (const Path& path : paths) {
        if (path.size() < 2)
            continue;

        sf::VertexArray lines(sf::PrimitiveType::LineStrip,
                              path.size());

        for (std::size_t i = 0; i < path.size(); ++i) {
            lines[i].position = path[i];
            lines[i].color = sf::Color::White;
        }

        window.draw(lines);
    }
}

void viewportScale(std::vector<Path>& paths)
{
    float minX = 99999.f;
    float maxX = -99999.f;
    float minY = 99999.f;
    float maxY = -99999.f;

    for (const Path& path : paths) {
        for (const sf::Vector2f& point : path) {
            minX = std::min(minX, point.x);
            maxX = std::max(maxX, point.x);
            minY = std::min(minY, point.y);
            maxY = std::max(maxY, point.y);
        }
    }

    float width = maxX - minX;
    float height = maxY - minY;
    float scale = std::max(width, height) / scaleMultiplier;

    if (width > 0 && height > 0) {
        float centerX = (minX + maxX) / 2.f;
        float centerY = (minY + maxY) / 2.f;

        for (Path& path : paths) {
            for (sf::Vector2f& point : path) {
                point.x = 2.f * (point.x - centerX) / scale;
                point.y = 2.f * (point.y - centerY) / scale;
            }
        }
    }

}


void renderingThread(sf::RenderWindow& window, std::atomic<bool>& running, std::atomic<bool>& contextReady)
{
    
    if (!window.setActive(true)) {
        std::cerr << "CRITICAL: Thread failed to claim context!" << std::endl;
        contextReady = true;
        return;
    }
    contextReady = true;

    // Initialize GLAD with SFML's function loader
    if (!gladLoadGL(sf::Context::getFunction)) {
        std::cerr << "Failed to initialize GLAD!" << std::endl;
        return;
    }

    // Hard verification check:
    if (sf::Shader::isAvailable()) {
        std::cout << "SUCCESS: Shaders are fully available on this thread!" << std::endl;
    } else {
        std::cerr << "ERROR: Context is active on thread, but driver reports NO SHADERS." << std::endl;
    }

    
    // match OpenGL's -1 to 1 canonical view with the Y axis pointing up.
	// Passing a negative height (-2.f) flips the Y-axis upside down!
	// Now: Top-Left is (-1.0, 1.0) and Bottom-Right is (1.0, -1.0)
    sf::View view(sf::Vector2f(viewX, 0.f), sf::Vector2f(2.f, -2.f));
	window.setView(view);

    // Background and alpha blending (for transparency)
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    while(running) {
        // draw...
		view.setCenter(sf::Vector2f(viewX, 0.f));
        window.setView(view);

        glClear(GL_COLOR_BUFFER_BIT);
		window.pushGLStates(); // Save OpenGL states before drawing SFML shapes
		drawGlyph(window, paths);
		window.popGLStates(); // Restore OpenGL states after drawing SFML shapes
		window.display();
    }

	window.setActive(false);
}


int main() {

	sf::ContextSettings settings;
    settings.majorVersion = 2;
    settings.minorVersion = 1;
    settings.attributeFlags = sf::ContextSettings::Default;
	
	sf::RenderWindow window(
    	sf::VideoMode({800, 800}), 
    	"SFML + GLAD (OpenGL 2.1)",
        sf::Style::Default, 
    	sf::State::Windowed,  
    	settings
	);



    // set additional window properties
    window.setKeyRepeatEnabled(false);
    
    // DO NOT LIMIT FRAME RATE! Use elapsed time to control updates.

    // deactivate its OpenGL context
    if( !window.setActive(false) ) {
        std::cerr << "Error setting up window, could not deactivate context" << std::endl;
        return -1;
    }

	// assets
    std::string text;
    std::getline(std::cin, text);


    float offset = 0;
    float space = 1.0;
    for (char c : text) {
        int num = c;
        std::string filename = "fonts/cmuntt/gly_" + std::to_string(num);
        std::vector<Path> glyphPaths = loadGlyph(filename);

        for (Path& path : glyphPaths) {
            for (sf::Vector2f& point : path) {
                point.x += offset;
            }
        }
        paths.insert(paths.end(), glyphPaths.begin(), glyphPaths.end());
        offset += space;
    }
    viewportScale(paths);


	std::atomic<bool> running(true);
    std::atomic<bool> contextReady(false);

    // Launch the rendering thread
    std::thread thread(renderingThread, std::ref(window), std::ref(running), std::ref(contextReady));
 
    // CRUCIAL: Pause the main thread until the rendering thread has successfully claimed the context.
    // This prevents pollEvent() from clashing with setActive(true) during initialization.
    while (!contextReady) {
        std::this_thread::yield(); 
    }

    // the event/logic/whatever loop
    while (window.isOpen()) {
		while ( const std::optional event = window.pollEvent() )
		{
			if ( event->is<sf::Event::Closed>() ) {
				running = false; 
                
                // Wait for the rendering thread to safely deactivate and exit
                if (thread.joinable()) {
                    thread.join();
                }

                // Do additional cleanup here before closing the window
                window.close();
			}

            if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                
                if (keyPressed->code == sf::Keyboard::Key::Up) {
                    std::cout << "Up arrow pressed!" << std::endl;
                    scaleMultiplier += 0.1;
                    viewportScale(paths);
                }
                else if (keyPressed->code == sf::Keyboard::Key::Down) {
                    std::cout << "Down arrow pressed!" << std::endl;
                    scaleMultiplier -= 0.1;
                    viewportScale(paths);
                }
                else if (keyPressed->code == sf::Keyboard::Key::Right) {
                    std::cout << "Right arrow pressed!" << std::endl;
                    viewX += 0.1f;
                }
                else if (keyPressed->code == sf::Keyboard::Key::Left) {
                    std::cout << "Left arrow pressed!" << std::endl;
                    viewX -= 0.1f;
                }

            }

            if (const auto* keyReleased = event->getIf<sf::Event::KeyReleased>()) {
                
				// key release events are used to trigger actions, not to hold them down.

			}
	
		}
    }

}