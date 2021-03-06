/**
 * Implementation of high-level functions to examine and manipulate game objects.
 * These functions are used by the Director to implement the game logic.
 */
#pragma once

#include "stage.hpp"
#include <unordered_set>

/**
 * Examines the pit for matching blocks from a sequence of �hot� blocks
 * which have just been moved or landed. They are passed to the MatchBuilder via ignite(). 
 * Returns all detected matching blocks (3 or more in a row from a hot block) in result().
 * The combo() specifies the number of blocks resolved at the same time.
 */
class MatchBuilder
{

private:

	/**
	 * Hashing helper struct to enable a std::unordered_set of blocks and garbage
	 */
	struct PhysHash
	{
		size_t operator()(const Physical& phys) const noexcept { return RowColHash{}(phys.rc()); }
	};

	/**
	 * Equality helper struct to enable a std::unordered_set of blocks and garbage
	 */
	struct PhysEqual
	{
		bool operator()(const Physical& lhs, const Physical& rhs) const noexcept { return lhs.rc() == rhs.rc(); }
	};

public:

	using BlockSet = std::unordered_set<std::reference_wrapper<Block>, PhysHash, PhysEqual>;
	using GarbageSet = std::unordered_set<std::reference_wrapper<Garbage>, PhysHash, PhysEqual>;

	MatchBuilder(const Pit& pit) : pit(pit), m_chaining(false) {}

	void ignite(Block& block);
	const BlockSet& result() { return m_result; }

	int combo() { return static_cast<int>(m_result.size()); }
	bool chaining() { return m_chaining; }

private:

	const Pit& pit;
	BlockSet m_result;
	bool m_chaining;

	bool match_at(RowCol rc, Block::Color color);
	void insert(RowCol rc);

};

using PhysicalRefVec = std::vector<std::reference_wrapper<Physical>>;
using BlockRefVec = std::vector<std::reference_wrapper<Block>>;
using GarbageRefVec = std::vector<std::reference_wrapper<Garbage>>;

class Logic
{

public:

	explicit Logic(Pit& pit) : m_pit(pit) {}

	/**
	 * Mark all objects at the given location and above as potentially falling.
	 */
	void trigger_falls(RowCol rc, bool chaining) const;

	/**
	 * Look at the pit contents and determine if any of the contents fulfill
	 * specific criteria.
	 *
	 * @param[out] chaining whether any block is currently marked as chaining
	 * @param[out] breaking whether any block is currently being dissolved
	 * @param[out] full whether any resting physical is up against the pit top
	 */
	void examine_pit(bool& chaining, bool& breaking, bool& full) const noexcept;

	/**
	 * Classify Physicals whose states are �running out�.
	 * For example, an object�s internal timer can run out while they are falling,
	 * indicating that they have reached their target location.
	 *
	 * @param[out] dead_physical Flag which indicates true if there are new dead physicals
	 * @param[out] dead_block Flag which indicates true if there are new dead blocks
	 * @param[out] dead_sound Flag which indicates true if there are non-fake dead blocks
	 * @param[out] chainstop Flag which indicates true if a chain might be finished
	 */
	void examine_finish(bool& dead_physical, bool& dead_block, bool& dead_sound, bool& chainstop) const;

	/**
	 * Shrink or remove expired garbage blocks.
	 * As a result, some physicals may be tagged with TAG_FALL.
	 */
	void convert_garbage() const;

	/**
	 * All physicals tagged with TAG_FALL now actually enter the *fall*
	 * state if possible.
	 * Successful fallers can not match and therefore have TAG_HOT removed.
	 */
	void handle_fallers() const;

	/**
	 * All matching blocks and all adjacent garbage bricks enter the *break* state.
	 *
	 * @param pit Pit object
	 * @param[out] have_match Flag which indicates true if there is at least one match
	 * @param[out] combo Counter for the number of blocks matched
	 * @param[out] chaining Flag which indicates true if there is a match involving chaining blocks
	 * @param[out] chainstop Flag which indicates true if chaining blocks have come to rest
	 */
	void handle_hots(bool& have_match, int& combo, bool& chaining, bool& chainstop) const;

private:

	Pit& m_pit;

	/**
	 * Mark the garbage and any other garbage it touches with the TAG_TOUCH tag.
	 */
	void touch_garbage(Garbage& garbage) const;

};
