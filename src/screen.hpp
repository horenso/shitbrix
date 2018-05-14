/**
 * screen.hpp
 * Screen class definitions
 * A screen is one (visual) state of the application and presents itself in a distinct design.
 */
#pragma once

#include "globals.hpp"
#include "input.hpp"
#include "stage.hpp"
#include "draw.hpp"
#include "audio.hpp"
#include "logic.hpp"
#include "director.hpp"
#include "replay.hpp"
#include "options.hpp"
#include "network.hpp"
#include "sdl_helper.hpp"
#include <fstream>
#include <typeinfo>

class IScreen : public IControllerSink
{
public:
	IScreen() = default;
	virtual ~IScreen() =0;

	// Screens are complex objects and can not be copied or moved.
	IScreen(const IScreen& ) = delete;
	IScreen(IScreen&& ) = delete;
	IScreen& operator=(const IScreen& ) = delete;
	IScreen& operator=(IScreen&& ) = delete;

	virtual void update() =0;
	virtual void draw(float dt) =0;

	virtual bool done() const =0; // whether the screen has ended

	/**
	 * Access the object which can draw this screen.
	 */
	virtual const IDraw& get_draw() const =0;

	virtual void input(ControllerInput cinput) =0;
	virtual void input_debug(int func) {} // developer help function
};

/**
 * Creates Screens.
 */
class ScreenFactory
{

public:

	ScreenFactory(const Options& options, const Assets& assets, const Audio& audio, ENetClient& client);

	std::unique_ptr<IScreen> create_menu();
	std::unique_ptr<IScreen> create_game();
	std::unique_ptr<IScreen> create_transition(IScreen& predecessor, IScreen& successor);

private:

	// resources to create the Screens
	const Options& m_options;
	const Assets& m_assets;
	const Audio& m_audio;
	ENetClient& m_client;

};

class PinkScreen; //!< debugging screen, never shown
class MenuScreen;
class GameScreen;
class TransitionScreen;

class PinkScreen : public IScreen
{

public:

	PinkScreen(PinkDraw&& draw) : m_draw(draw) {}
	virtual void update() override {}
	virtual void draw(float dt) override { m_draw.draw(dt); }
	virtual bool done() const override { return m_done; }
	virtual const IDraw& get_draw() const override { return m_draw; }
	virtual void input(ControllerInput cinput) override { if(Button::A == cinput.button && ButtonAction::DOWN == cinput.action) m_done = true; }

private:

	PinkDraw m_draw;
	bool m_done = false;

};

class MenuScreen : public IScreen
{

public:

	enum class Result { PLAY, QUIT };

	MenuScreen(DrawMenu&& draw, const Audio& sound);

	virtual void update() override;
	virtual void draw(float dt) override;
	virtual bool done() const override { return m_done; }
	virtual const IDraw& get_draw() const override { return m_draw; }
	virtual void input(ControllerInput cinput) override;

	/**
	 * Return the result of the MenuScreen.
	 * It is an error to ask for this before the screen is done().
	 */
	Result result() const { enforce(m_done); return m_result; }

private:

	long m_game_time; //!< starts at 0 and increases with update()
	bool m_done; //!< true if this screen has reached its end
	Result m_result; //!< valid only when m_done

	DrawMenu m_draw; //!< Interface for drawing the screen

};

/**
 * Determines some of the variable behavior of the GameScreen (strategy pattern).
 * The GameScreen starts with an intro (fade-in or transition), continues with the
 * actual gameplay and ends with a result banner. All of these are implemented as
 * strategies derived from GamePhase.
 * GamePhases control their own life and transition via GameScreen::set_phase(),
 * enabled through the friend relation to GameScreen.
 */
class IGamePhase
{

public:

	IGamePhase(GameScreen* screen) : m_screen(screen) {}
	virtual ~IGamePhase() =0;

	void set_screen(GameScreen* screen) { m_screen = screen; }

	virtual void update() =0;

protected:

	GameScreen* m_screen;

};

class GameIntro : public IGamePhase
{

public:

	GameIntro(GameScreen* screen);

	virtual void update() override;

private:

	static constexpr int INTRO_TIME = 20; // number of animation frames for intro
	int countdown;

};

class GamePlay : public IGamePhase
{

public:

	GamePlay(GameScreen* screen) : IGamePhase(screen) {}

	virtual void update() override;

};

class GameResult : public IGamePhase
{

public:

	GameResult(GameScreen* screen, int winner);

	virtual void update() override;

};

class GameScreen : public IScreen
{

public:

	explicit GameScreen(DrawGame&& draw, const Audio& audio, Journal& journal, ENetClient& client);
	virtual ~GameScreen() noexcept;

	virtual void update() override;
	virtual void draw(float dt) override;
	virtual bool done() const override { return m_done; }
	virtual const IDraw& get_draw() const override { return m_draw; }
	virtual void input(ControllerInput cinput) override;

private:

	long m_game_time; // starts at 0 with each game round
	bool m_done; // true if this screen has reached its end
	bool m_pause; // true if tick updates are supressed

	std::unique_ptr<IGamePhase> m_game_phase;
	std::unique_ptr<IGamePhase> m_next_phase;

	std::unique_ptr<Stage> m_stage;
	DrawGame m_draw;
	Journal& m_journal;
	ENetClient& m_client;
	evt::SoundRelay m_sound_relay;
	ShakeRelay m_shake_relay;
	std::unique_ptr<evt::GameOverRelay> m_gameover_relay;
	Rules m_rules;

	void change_phase(std::unique_ptr<IGamePhase> phase);
	void change_phase_impl();

	void start();

	/**
	 * Pass on the update event to child objects.
	 */
	void update_impl();

	friend class IGamePhase;
	friend class GameIntro;
	friend class GamePlay;
	friend class GameResult;

};

class TransitionScreen : public IScreen
{

public:

	TransitionScreen(IScreen& predecessor, IScreen& successor, DrawTransition&& draw)
	: m_predecessor(predecessor), m_successor(successor), m_time(0), m_draw(std::move(draw))
	{}

	virtual void update() override;
	virtual void draw(float dt) override;
	virtual bool done() const override { return m_time >= TRANSITION_TIME; }
	virtual const IDraw& get_draw() const override { return m_draw; }
	virtual void input(ControllerInput cinput) override { m_successor.input(cinput); }

	IScreen& successor() const { return m_successor; }

private:

	IScreen& m_predecessor;
	IScreen& m_successor;
	int m_time; //!< starts at 0 and increases with update()
	DrawTransition m_draw;

};
