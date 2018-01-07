/**
 * Tests for the game logic implementation in BlockDirector.
 */

#include "stage.hpp"
#include "director.hpp"
#include "gtest/gtest.h"

namespace
{

/**
 * Properly generates a block falling from the given coordinates.
 */
Block& spawn_falling_block(Pit& pit, Block::Color color, RowCol from);

}

class BlockDirectorTest : public ::testing::Test
{

protected:

	virtual void SetUp()
	{
		pit = std::make_unique<Pit>(Point{0,0});

		// 1 preview row, 2 normal rows, 1 half row, match-ready
		pit->spawn_block(Block::Color::BLUE, RowCol{0, 0}, Block::State::REST);
		pit->spawn_block(Block::Color::RED, RowCol{0, 1}, Block::State::REST);
		pit->spawn_block(Block::Color::YELLOW, RowCol{0, 2}, Block::State::REST);
		pit->spawn_block(Block::Color::GREEN, RowCol{0, 3}, Block::State::REST);
		pit->spawn_block(Block::Color::PURPLE, RowCol{0, 4}, Block::State::REST);
		pit->spawn_block(Block::Color::ORANGE, RowCol{0, 5}, Block::State::REST);

		pit->spawn_block(Block::Color::ORANGE, RowCol{-1, 0}, Block::State::REST);
		pit->spawn_block(Block::Color::BLUE, RowCol{-1, 1}, Block::State::REST);
		pit->spawn_block(Block::Color::RED, RowCol{-1, 2}, Block::State::REST);
		pit->spawn_block(Block::Color::YELLOW, RowCol{-1, 3}, Block::State::REST);
		pit->spawn_block(Block::Color::GREEN, RowCol{-1, 4}, Block::State::REST);
		pit->spawn_block(Block::Color::PURPLE, RowCol{-1, 5}, Block::State::REST);

		pit->spawn_block(Block::Color::BLUE, RowCol{-2, 0}, Block::State::REST);
		pit->spawn_block(Block::Color::RED, RowCol{-2, 1}, Block::State::REST);
		pit->spawn_block(Block::Color::YELLOW, RowCol{-2, 2}, Block::State::REST);
		pit->spawn_block(Block::Color::GREEN, RowCol{-2, 3}, Block::State::REST);
		pit->spawn_block(Block::Color::PURPLE, RowCol{-2, 4}, Block::State::REST);
		pit->spawn_block(Block::Color::ORANGE, RowCol{-2, 5}, Block::State::REST);

		pit->spawn_block(Block::Color::RED, RowCol{-3, 2}, Block::State::REST);
		pit->spawn_block(Block::Color::YELLOW, RowCol{-3, 3}, Block::State::REST);
		pit->spawn_block(Block::Color::GREEN, RowCol{-3, 4}, Block::State::REST);

		const int SEED = 0;
		rndgen = std::make_shared<std::mt19937>(SEED);
		director = std::make_unique<BlockDirector>(*pit, rndgen);
	}

	// virtual void TearDown() {}

	void run_game_ticks(int ticks)
	{
		for(int t = 0; t < ticks; t++) {
			pit->update();
			director->update();
		}
	}

	std::unique_ptr<Pit> pit;
	RndGen rndgen;
	std::unique_ptr<BlockDirector> director;

};

/**
 * Tests whether blocks correctly cause a match when one lands next
 * to others of the same color.
 */
TEST_F(BlockDirectorTest, LandAndMatch)
{
	auto& top_block = spawn_falling_block(*pit, Block::Color::RED, RowCol{-7, 2});
	auto& mid_block = spawn_falling_block(*pit, Block::Color::RED, RowCol{-5, 2});

	// wait until blocks landed and match
	const int FALL_T = std::ceil(static_cast<float>(ROW_HEIGHT)*2/FALL_SPEED);
	run_game_ticks(FALL_T);

	EXPECT_EQ(Block::State::BREAK, top_block.block_state());
	EXPECT_EQ(Block::State::BREAK, mid_block.block_state());

	RowCol top_final_rc = top_block.rc();
	RowCol mid_final_rc = mid_block.rc();

	run_game_ticks(BREAK_TIME);
	EXPECT_FALSE(pit->at(top_final_rc)); // matched blocks are gone
	EXPECT_FALSE(pit->at(mid_final_rc));
}

/**
 * Tests whether blocks correctly cause a match when one lands next
 * to others of the same color. This test is more rigorous than LandAndMatch.
 */
TEST_F(BlockDirectorTest, HorizontalMatch)
{
	pit->spawn_block(Block::Color::RED, RowCol{-3, 0}, Block::State::REST);
	auto& fall_block = pit->spawn_block(Block::Color::RED, RowCol{-4, 2}, Block::State::REST);
	const RowCol swap_target_rc{-4,1};
	bool swapping = director->swap(swap_target_rc);
	ASSERT_TRUE(swapping);
	ASSERT_EQ(swap_target_rc, fall_block.rc());
	ASSERT_EQ(Block::State::SWAP_LEFT, fall_block.block_state());

	// wait until block has swapped above the gap
	const int SWAP_T = SWAP_TIME;
	ASSERT_EQ(SWAP_T, fall_block.eta());
	run_game_ticks(SWAP_T-1);
	EXPECT_EQ(swap_target_rc, fall_block.rc());
	EXPECT_EQ(Block::State::SWAP_LEFT, fall_block.block_state());
	run_game_ticks(1);
	const RowCol fall_target_rc{-3,1};
	EXPECT_EQ(fall_target_rc, fall_block.rc());
	EXPECT_EQ(Block::State::FALL, fall_block.block_state());

	// wait until block lands and matches
	const int FALL_T = std::ceil(static_cast<float>(ROW_HEIGHT)/FALL_SPEED);
	// EXPECT_EQ(FALL_T, fall_block.time); // NOTE: falling does not run on time (yet)
	run_game_ticks(FALL_T-1);
	EXPECT_EQ(Block::State::FALL, fall_block.block_state());
	run_game_ticks(1);
	EXPECT_EQ(Block::State::BREAK, fall_block.block_state());

	const int BREAK_T = BREAK_TIME;
	EXPECT_EQ(BREAK_T, fall_block.eta());
	run_game_ticks(BREAK_T-1);
	EXPECT_EQ(1, fall_block.eta());
	run_game_ticks(1);

	// matched blocks are gone
	EXPECT_FALSE(pit->at(RowCol{-3,0}));
	EXPECT_FALSE(pit->at(RowCol{-3,1}));
	EXPECT_FALSE(pit->at(RowCol{-3,2}));
}

/**
 * Tests whether garbage blocks correctly dissolve when
 * hit by a nearby block match.
 */
TEST_F(BlockDirectorTest, DissolveGarbage)
{
	auto& garbage = pit->spawn_garbage(RowCol{-5, 0}, 6, 2); // chain garbage
	garbage.set_state(Physical::State::REST);

	RowCol lrc = RowCol{-2,2};
	RowCol rrc = RowCol{-2,3};
	auto& left_block = *pit->block_at(lrc);
	auto& right_block = *pit->block_at(rrc);

	// 3 in a row
	left_block.set_state(Block::State::SWAP_RIGHT, SWAP_TIME);
	right_block.set_state(Block::State::SWAP_LEFT, SWAP_TIME);
	pit->swap(left_block, right_block);

	const int DISSOLVE_T = 52; // ticks until block landed, garbage has shrunk, blocks have fallen down
	run_game_ticks(DISSOLVE_T);

	EXPECT_EQ(1, garbage.rows());
	EXPECT_FALSE(pit->garbage_at(RowCol{-4, 3})); // garbage shrunk
	EXPECT_TRUE(pit->block_at(RowCol{-4, 2})); // block remains
	EXPECT_FALSE(pit->block_at(RowCol{-4, 0})); // this block should be falling
	EXPECT_TRUE(pit->block_at(RowCol{-3, 0})); // down to here
}

/**
 * Tests whether blocks spawned from a dissolving garbage correctly fall down.
 * In particular, there is an issue when blocks are supposed to fall where the
 * garbage-touching match blocks are being removed.
 */
TEST_F(BlockDirectorTest, DissolveAndFall)
{
	auto& garbage = pit->spawn_garbage(RowCol{-5, 0}, 6, 2); // chain garbage
	garbage.set_state(Physical::State::REST);

	RowCol lrc = RowCol{-2,2};
	RowCol rrc = RowCol{-2,3};
	auto& left_block = *pit->block_at(lrc);
	auto& right_block = *pit->block_at(rrc);

	// 3 in a row
	left_block.set_state(Block::State::SWAP_RIGHT, SWAP_TIME);
	right_block.set_state(Block::State::SWAP_LEFT, SWAP_TIME);
	pit->swap(left_block, right_block);

	// ticks until block landed, garbage has shrunk, blocks have fallen down
	const int DISSOLVE_T = SWAP_TIME + DISSOLVE_TIME + 2;
	run_game_ticks(DISSOLVE_T);

	EXPECT_FALSE(pit->at(rrc)); // blocks have matched away
	EXPECT_FALSE(pit->block_at(RowCol{-4, 3})); // this block has fallen
	EXPECT_TRUE(pit->block_at(RowCol{-3, 3})); // down to here
}

/**
 * Tests whether a partially dissolved garbage block itself correctly falls
 * down when there is immediately no support to hold it up after dissolving it.
 */
TEST_F(BlockDirectorTest, FallAfterShrink)
{
	RowCol garbage_rc{-6,0};
	auto& garbage = pit->spawn_garbage(garbage_rc, 6, 2); // chain garbage
	garbage.set_state(Physical::State::REST);

	// vertical match just under the garbage
	pit->spawn_block(Block::Color::YELLOW, RowCol{-4, 2}, Block::State::REST);

	RowCol lrc = RowCol{-3,2};
	RowCol rrc = RowCol{-3,3};
	auto& left_block = *pit->block_at(lrc);
	auto& right_block = *pit->block_at(rrc);

	// 3 in a row
	left_block.set_state(Block::State::SWAP_RIGHT, SWAP_TIME);
	right_block.set_state(Block::State::SWAP_LEFT, SWAP_TIME);
	pit->swap(left_block, right_block);

	// ticks until blocks swapped, garbage shrunk, blocks have started to fall down
	const int DISSOLVE_T = SWAP_TIME + DISSOLVE_TIME + 2;
	run_game_ticks(DISSOLVE_T);

	EXPECT_EQ(Physical::State::FALL, garbage.physical_state()); // garbage has moved
	EXPECT_FALSE(pit->garbage_at(garbage_rc)); // garbage has fallen
	EXPECT_EQ(&garbage, pit->garbage_at(RowCol{-5, 3})); // down to here
}

/**
 * Tests whether a swapping block correctly falls down after it arrives in a
 * space with nothing below. All blocks above must fall with it.
 */
TEST_F(BlockDirectorTest, FallAfterSwap)
{
	// This is the block that is going to do the swapping (to the right).
	// At the last moment before it completes the swap, a green block lands
	// on the red block. The red block notices it doesn’t have ground and falls.
	// The green block immediately falls with it.
	Block& red_block = pit->spawn_block(Block::Color::RED, RowCol{-4, 4}, Block::State::REST);
	Block* green_block = nullptr;
	bool swapping = false;

	const int SWAP_T = SWAP_TIME;
	const int FALL_T = std::ceil(static_cast<float>(ROW_HEIGHT)/FALL_SPEED);
	const int LAND_MOMENT = std::max(SWAP_T, FALL_T) + 1;
	const int SWAP_START = LAND_MOMENT - SWAP_T;
	const int SPAWN_MOMENT = LAND_MOMENT - FALL_T - 1;

	for(int t = 0; t < LAND_MOMENT; t++) {
		if(SWAP_START == t) {
			swapping = director->swap(RowCol{-4,4});
		}
		if(SPAWN_MOMENT == t) {
			green_block = &spawn_falling_block(*pit, Block::Color::GREEN, RowCol{-6, 5});
		}

		if(LAND_MOMENT-1 == t) {
			EXPECT_EQ(1, red_block.eta());
			EXPECT_EQ(Block::State::SWAP_RIGHT, red_block.block_state());
			EXPECT_EQ(Block::State::LAND, green_block->block_state());
		}

		run_game_ticks(1);
	}

	// both events must have occurred
	ASSERT_TRUE(swapping);
	ASSERT_TRUE(green_block);

	// both blocks are now falling
	RowCol expected_red_rc{-3,5};
	RowCol expected_green_rc{-4,5};
	EXPECT_EQ(expected_red_rc, red_block.rc());
	EXPECT_EQ(Block::State::FALL, red_block.block_state());
	EXPECT_EQ(expected_green_rc, green_block->rc());
	EXPECT_EQ(Block::State::FALL, green_block->block_state());
}

/**
 * Tests whether a block falling down from above a completed match
 * is correctly marked as chaining by the director.
 * When the falling blocks come to rest, they are no longer chaining.
 */
TEST_F(BlockDirectorTest, ChainingFallBlock)
{
	Block* red_block = pit->block_at(RowCol{-3,2}); // to fall down
	ASSERT_TRUE(red_block);

	bool swapping = director->swap(RowCol{-1,2}); // match yellow blocks vertically
	ASSERT_TRUE(swapping);

	// wait until the yellow blocks have cleared and the red one falls down
	const int PRERUN_T = SWAP_TIME + BREAK_TIME;
	run_game_ticks(PRERUN_T);

	RowCol expected_rc{-2,2};
	EXPECT_EQ(expected_rc, red_block->rc());
	EXPECT_EQ(Block::State::FALL, red_block->block_state());
	EXPECT_TRUE(red_block->chaining);

	const int FALL_T = std::ceil(static_cast<float>(ROW_HEIGHT*3) / FALL_SPEED);
	run_game_ticks(FALL_T);
	expected_rc = RowCol{0,2};
	EXPECT_EQ(expected_rc, red_block->rc());
	EXPECT_EQ(Block::State::LAND, red_block->block_state());
	EXPECT_FALSE(red_block->chaining);
}

/**
 * Tests whether a block falling down from a dissolved garbage
 * is correctly marked as chaining by the director.
 */
TEST_F(BlockDirectorTest, ChainingGarbageBlock)
{
	const int GARBAGE_COLS = 6;
	auto& garbage = pit->spawn_garbage(RowCol{-5, 0}, GARBAGE_COLS, 2); // chain garbage
	garbage.set_state(Physical::State::REST);
	bool swapping = director->swap(RowCol{-2,2}); // match yellow blocks vertically
	ASSERT_TRUE(swapping);

	// ticks until block landed, garbage has shrunk, blocks have fallen down
	const int DISSOLVE_T = SWAP_TIME + DISSOLVE_TIME;
	run_game_ticks(DISSOLVE_T);

	auto expect_chaining = [this] (RowCol rc, bool expected) {
		Block* block = pit->block_at(rc);
		ASSERT_TRUE(block);
		EXPECT_EQ(expected, block->chaining);
	};

	// Those blocks from the garbage which land on top of resting blocks
	// and do not enter a match will also immediately stop chaining
	expect_chaining(RowCol{-3, 0}, true);
	expect_chaining(RowCol{-3, 1}, true);
	expect_chaining(RowCol{-4, 2}, false);
	expect_chaining(RowCol{-3, 3}, true);
	expect_chaining(RowCol{-4, 4}, false);
	expect_chaining(RowCol{-3, 5}, true);
}

/**
 * Tests whether block swapping correctly swaps the chaining markers
 * of the blocks, even if it happens mid-fall.
 */
TEST_F(BlockDirectorTest, ChainingSwapBlock)
{
	Block* red_block = pit->block_at(RowCol{-3,2}); // to fall down
	ASSERT_TRUE(red_block);

	bool swapping = director->swap(RowCol{-1,2}); // match yellow blocks vertically
	ASSERT_TRUE(swapping);

	const int BREAK_T = SWAP_TIME + BREAK_TIME;
	run_game_ticks(BREAK_T);

	ASSERT_EQ(Block::State::FALL, red_block->block_state());

	const int FALL_T = std::ceil(static_cast<float>(ROW_HEIGHT*2)/FALL_SPEED) + 1;
	run_game_ticks(FALL_T);

	RowCol expected_rc{0,2};
	ASSERT_EQ(expected_rc, red_block->rc());
	ASSERT_EQ(Block::State::FALL, red_block->block_state());
	EXPECT_TRUE(red_block->chaining);

	Block* green_block = pit->block_at(RowCol{0,3});
	ASSERT_TRUE(green_block);

	swapping = director->swap(RowCol{0,2}); // skill-chain move
	ASSERT_TRUE(swapping);
	EXPECT_FALSE(red_block->chaining);
	EXPECT_TRUE(green_block->chaining);
}

/**
 * Tests whether the director honors panic time to stave off game over.
 */
TEST_F(BlockDirectorTest, PanicSimple)
{
	// complete the test scenario with a block pillar almost to the top
	pit->spawn_block(Block::Color::RED, RowCol{-4, 3}, Block::State::REST);
	pit->spawn_block(Block::Color::YELLOW, RowCol{-5, 3}, Block::State::REST);
	pit->spawn_block(Block::Color::GREEN, RowCol{-6, 3}, Block::State::REST);
	pit->spawn_block(Block::Color::PURPLE, RowCol{-7, 3}, Block::State::REST);
	pit->spawn_block(Block::Color::ORANGE, RowCol{-8, 3}, Block::State::REST);

	// time it takes for the orange block to reach the top of the pit
	const int TIME_TO_FULL = ROW_HEIGHT / SCROLL_SPEED;

	// moment before panic
	run_game_ticks(TIME_TO_FULL);
	ASSERT_FALSE(director->is_panic());
	ASSERT_FALSE(director->over());

	// enter panic
	run_game_ticks(1);
	ASSERT_TRUE(director->is_panic());
	ASSERT_FALSE(director->over());

	// before panic depleted
	run_game_ticks(PANIC_TIME - 1);
	ASSERT_TRUE(director->is_panic());
	ASSERT_FALSE(director->over());

	// really over
	run_game_ticks(1);
	ASSERT_TRUE(director->is_panic());
	ASSERT_TRUE(director->over());
}


namespace
{

Block& spawn_falling_block(Pit& pit, Block::Color color, RowCol from)
{
	// A falling block really belongs on the next row, where it expects
	// to arrive from the fall.
	from.r++;

	// We set a block in motion by @c set_state. At create time it rests.
	Block& block = pit.spawn_block(color, from, Block::State::REST);
	block.set_state(Block::State::FALL, ROW_HEIGHT, FALL_SPEED);

	return block;
}

}
