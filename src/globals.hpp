/**
 * globals.hpp
 * General global definitions without dependencies.
 * Every other header may include this header.
 */

#pragma once

#include <ostream> // debug stuff

// ================================================
// Enumeration types and constants
// ================================================

/**
 * IDs for all the gfx assets.
 * One gfx can refer to several frames or states of the object.
 */
enum class Gfx
{
	BACKGROUND = 0,
	BLOCK_BLUE,
	BLOCK_RED,
	BLOCK_YELLOW,
	BLOCK_GREEN,
	BLOCK_PURPLE,
	BLOCK_ORANGE,
	PITVIEW,  // debug gfx
	CURSOR,
	BANNER,
	GARBAGE_LU,
	GARBAGE_U,
	GARBAGE_RU,
	GARBAGE_L,
	GARBAGE_M,
	GARBAGE_R,
	GARBAGE_LD,
	GARBAGE_D,
	GARBAGE_RD,
	BONUS,
	MENUBG
};

/**
 * IDs for all the sound effect assets.
 */
enum class Snd
{
	SWAP = 0, // swap blocks (click)
	BREAK,    // break blocks (splat)
	MATCH,    // match blocks (ding)
	LANDING,  // smashing block landing (thump)
	CONFIRM,  // menu confirm (cheerful ding)
	DECLINE,  // menu decline (disappointed ding)
	START,    // game start (shot or fireworks launch)
	END,      // game end (alarming crumble)
	RESULT    // game over (cheer)
};

// Allow operator+ on Gfx
Gfx operator+(Gfx gfx, int delta);

enum class BlockFrame : size_t
{
	REST = 0,
	PREVIEW = 1,
	BREAK_BEGIN = 2, // sequence of break anim
	BREAK_END = 6    // 1-past-end index
};

enum class BonusFrame : size_t
{
	COMBO,
	CHAIN
};

// Allow prefix operator++ on BlockFrame
BlockFrame& operator++(BlockFrame& frame);

/**
 * Direction, used for moving cursor
 */
enum class Dir { NONE, LEFT, RIGHT, UP, DOWN };

/**
 * All input actions that the game accepts at any point from one source,
 * after key mapping from the original input device (e.g. keyboard).
 * Direction values can be cast to and from Dir.
 */
enum class Button
{
	NONE,          // no button was pressed
	LEFT, RIGHT, UP, DOWN, // directional pad
	A, B,          // standard action buttons
	PAUSE,         // pause the game
	RESET,         // restart the game
	QUIT,          // exit the game
	DEBUG1,        // debug functions
	DEBUG2,
	DEBUG3,
	DEBUG4,
	DEBUG5
};

/**
 * Enumeration of possible input actions by one player.
 * These are also the possible actions from a replay file.
 * Direction values can be cast to and from Dir.
 * All values can be cast to and from Button.
 */
enum class GameButton { NONE, LEFT, RIGHT, UP, DOWN, SWAP, RAISE };

/**
 * Return the string representation of the @c GameButton.
 */
const char* game_button_to_string(GameButton button) noexcept;

/**
 * Return the corresponding @c GameButton for the string representation.
 * @throw GameException if the string is not recognized.
 */
GameButton string_to_game_button(const std::string& button_string);

/**
 * Enumeration of the sorts of inputs that the player can perform on a button.
 * For some buttons (e.g. PAUSE), only DOWN may be registered.
 */
enum class ButtonAction { DOWN, UP };

/**
 * Return the string representation of the @c ButtonAction.
 */
const char* button_action_to_string(ButtonAction action) noexcept;

/**
 * Return the corresponding @c ButtonAction for the string representation.
 * @throw GameException if the string is not recognized.
 */
ButtonAction string_to_button_action(const std::string& action_string);

// ================================================
// Elemental utility structures
// ================================================

/**
 * Represents a screen location in canvas pixels.
 * {0,0} top left - {CANVAS_W,CANVAS_H} bottom right
 */
struct Point
{
	float x, y;

	Point offset(float dx, float dy) const { return Point{x+dx, y+dy}; }
};

/**
 * Represents a block-sized space in one of the pits.
 * row 0 = base line (lowest line at the start)
 * row -9 = top of screen at the start
 * column 0 = leftmost column
 */
struct RowCol
{
	int r, c;

	bool operator==(const RowCol& rhs) const { return r == rhs.r && c == rhs.c; }
};

Point from_rc(RowCol rc); // conversion to pit-relative coordinates

/**
 * Hash function for @c RowCol to use with `std::unordered_map`.
 */
struct RowColHash
{
	size_t operator()(RowCol rc) const noexcept { return (rc.c << 16) + rc.r; }
};

std::ostream& operator<<(std::ostream& stream, RowCol rc);

/**
 * Holds one button input and the number of the player who pressed it.
 */
struct ControllerInput
{
	int player; // 0-based player index
	Button button;
	ButtonAction action;
};

/**
 * Holds one in-game action and the number of the player who pressed it.
 */
struct GameInput
{
	static const long TIME_ASAP = -1; //!< this input should be part of the next update

	long game_time;      //!< time when this input takes effect
	int player;          //!< 0-based player index
	GameButton button;
	ButtonAction action;

	/**
	 * Since @c GameInputs frequently need to be sent over the network or stored
	 * in a replay file, they have a canonical string representation.
	 */
	std::string to_string() const;

	/**
	 * Return the @c GameInput from the string representation.
	 * @throw GameException if the string is not recognized.
	 */
	static GameInput from_string(std::string input_string);
};

// ================================================
// Application constants
// ================================================

constexpr int TPS = 30; // fixed number of logic ticks per second (game speed)
constexpr long CHECKPOINT_INTERVAL = 1 * TPS; //!< time between checkpoints for journal
constexpr size_t MAX_CLIENTS = 8; //!< maximum number of networked players
constexpr uint16_t DEFAULT_PORT = 2414; //!< network port for connections
constexpr uint32_t CONNECT_TIMEOUT = 5000; //!< peer to server connection time limit
constexpr uint8_t MESSAGE_CHANNEL = 1; //!< network communication channel for gameplay messages

// Gameplay constants
constexpr int PIT_COLS = 6; //!< number of blocks that fit in a pit next to each other
constexpr int PIT_ROWS = 10; //!< number of blocks that fit in a pit on top of each other
constexpr int ROW_HEIGHT = 200; //!< gameplay height of a row; determines scroll speed etc.
constexpr int FALL_SPEED = 35; //!< points per update that a falling block moves down
constexpr int SCROLL_SPEED = 1; //!< points per update that the pit moves up
constexpr int RAISE_SPEED = 15; //!< pit speed when raising the stack (max speed)
constexpr int INTRO_TIME = 20; // number of ticks for intro to game round
constexpr int SWAP_TIME = 6; //!< number of ticks to swap two blocks
constexpr int BREAK_TIME = 30; //!< number of ticks for a block to break
constexpr int DISSOLVE_TIME = 30; //!< number of ticks for a garbage brick to dissolve
constexpr int LAND_TIME = 20; //!< number of ticks in an object’s landing state
constexpr int RECOVERY_TIME = 50; //!< number of ticks in which scrolling stops after quality match
constexpr int PANIC_TIME = 90; //!< number of ticks until game over when the pit is full
constexpr int NOONE = -1; //!< not-a player id

// Presentation constants (graphics, animation, sounds)
constexpr const char* APP_NAME = "shitbrix";
constexpr int FPS = 60; //!< aspired-to number of drawn and displayed frames per second
constexpr int AUDIO_SAMPLES = 4096;

constexpr int CANVAS_W = 640; //!< width of drawing canvas in pixels
constexpr int CANVAS_H = 480; //!< height of drawing canvas in pixels
constexpr int BLOCK_W = 40; //!< width of one little colored block
constexpr int BLOCK_H = 40; //!< height of one little colored block
constexpr int GARBAGE_W = BLOCK_W/2; //!< width of one drawable piece of garbage
constexpr int GARBAGE_H = BLOCK_H/2; //!< height of one drawable piece of garbage
constexpr int CURSOR_W = 88; //!< width of the cursor texture
constexpr int CURSOR_H = 48; //!< height of the cursor texture
constexpr int BONUS_W = 16; //!< width of the combo/chain star
constexpr int BONUS_H = 16; //!< height of the combo/chain star
constexpr float SHAKE_SCALE = 10.f; //!< pixel amount of shaking per row of garbage dropped
constexpr float SHAKE_DECREASE = .6f; //!< scale factor for shake strength per frame

constexpr Point LPIT_LOC = { 32, 48 };
constexpr Point RPIT_LOC = { 368, 48 };
constexpr Point LBONUS_LOC = { 320-32-5, 400 };
constexpr Point RBONUS_LOC = { 320+5, 400 };
constexpr int COL_W = BLOCK_W; //!< pixel width of a single column in the pit
constexpr int ROW_H = BLOCK_H; //!< pixel height of a single row in the pit
constexpr int PIT_W = PIT_COLS*COL_W; //!< width of the pit in canvas pixels
constexpr int PIT_H = PIT_ROWS*ROW_H; //!< height of the pit in canvas pixels

constexpr int BANNER_W = 200; //!< width of the win/lose banner in canvas pixels
constexpr int BANNER_H = 140; //!< height of the win/lose banner in canvas pixels

constexpr int TRANSITION_TIME = 20; //!< Number of frames for screen transition

// ================================================
// Global types and shared structures
// ================================================

/**
 * Holds meta-information about a game round.
 * This information does not change over time like the @c GameState does.
 * It is also used to generate the initial game state and reproduce the replay.
 */
struct GameMeta
{
	int players;   //!< number of participant players
	unsigned seed; //!< initial random seed
	int winner = NOONE; //!< player who won the game

	/**
	 * Since @c GameMetas need to be sent over the network and stored
	 * in a replay file, they have a canonical string representation.
	 */
	std::string to_string() const;

	/**
	 * Return the @c GameMeta from the string representation.
	 * @throw GameException if the string is not recognized.
	 */
	static GameMeta from_string(std::string meta_string);
};

/**
 * These dials contain general parameters that govern the current game session
 * outside the journal record of the game. They can be manipulated by the
 * server.
 */
struct Dials
{
	int speed = 1; //!< display speed of the game (currently just 0 for pause and 1 normally)
};

// ================================================
// Miscellaneous
// ================================================

/**
 * Set the current thread name.
 * Since we use libraries like SDL, which spawns a ridiculous number of its own
 * threads, we name our threads so that we can find them in the list when
 * debugging.
 */
void set_thread_name(const char* thread_name);
