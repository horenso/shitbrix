/**
 * Tests for the director’s usage of the IGameEvent interface.
 */

#include "stage.hpp"
#include "director.hpp"
#include "gameevent.hpp"
#include "mock.hpp"
#include "gtest/gtest.h"

class GameEventCounter : public evt::IGameEvent
{

public:

	virtual void fire(evt::CursorMoves moved) override { countCursorMoves++; }
	virtual void fire(evt::Swap swapped) override { countSwap++; }
	virtual void fire(evt::Match matched) override { last_match = matched; }
	virtual void fire(evt::Chain chained) override { last_chain = chained; }
	virtual void fire(evt::BlockDies died) override { countBlockDies++; }
	virtual void fire(evt::GarbageDissolves dissolved) override { countGarbageDissolves++; }

	int countCursorMoves = 0;
	int countSwap = 0;
	evt::Match last_match{0, false};
	evt::Chain last_chain{0};
	int countBlockDies = 0;
	int countGarbageDissolves = 0;

};

class GameEventTest : public ::testing::Test
{

protected:

	virtual void SetUp()
	{
		pit = std::make_unique<Pit>(Point{0,0});

		cursor = std::make_unique<Cursor>(RowCol{0,0});
		cursor_director = std::make_unique<CursorDirector>(*pit, *cursor);

		const int SEED = 0;
		rndgen = std::make_shared<std::mt19937>(SEED);
		block_director = std::make_unique<BlockDirector>(*pit, rndgen);

		counter = std::make_unique<GameEventCounter>();
		hub = std::make_unique<evt::GameEventHub>();
		hub->append(*counter);
		cursor_director->set_handler(*hub);
		block_director->set_handler(*hub);
	}

	// virtual void TearDown() {}

	void run_game_ticks(int ticks)
	{
		for(int t = 0; t < ticks; t++) {
			pit->update(context);
			block_director->update();
		}
	}

	MockContext context;
	std::unique_ptr<Pit> pit;
	std::unique_ptr<Cursor> cursor;
	RndGen rndgen;
	std::unique_ptr<CursorDirector> cursor_director;
	std::unique_ptr<BlockDirector> block_director;
	std::unique_ptr<GameEventCounter> counter;
	std::unique_ptr<evt::GameEventHub> hub;

};

/**
 * Tests whether a cursor move event is correctly generated by the CursorDirector.
 */
TEST_F(GameEventTest, CursorMoves)
{
	cursor_director->move(Dir::RIGHT);
	EXPECT_EQ(1, counter->countCursorMoves);
	cursor_director->move(Dir::NONE);
	EXPECT_EQ(1, counter->countCursorMoves);
}

/**
 * Tests whether a blocks swap event is correctly generated by the BlockDirector.
 */
TEST_F(GameEventTest, Swap)
{
	pit->spawn_block(Block::Color::BLUE, RowCol{0, 0}, Block::State::REST);
	pit->spawn_block(Block::Color::RED, RowCol{0, 1}, Block::State::REST);

	block_director->swap(RowCol{0,0});
	EXPECT_EQ(1, counter->countSwap);
	block_director->swap(RowCol{0,1});
	EXPECT_EQ(2, counter->countSwap);
	block_director->swap(RowCol{-1,1});
	EXPECT_EQ(2, counter->countSwap);
}

/**
 * Tests whether a match event is correctly generated by the BlockDirector.
 */
TEST_F(GameEventTest, Match)
{
	pit->spawn_block(Block::Color::BLUE, RowCol{0, 0}, Block::State::REST);
	pit->spawn_block(Block::Color::BLUE, RowCol{0, 1}, Block::State::REST);
	pit->spawn_block(Block::Color::RED, RowCol{0, 2}, Block::State::REST);
	pit->spawn_block(Block::Color::BLUE, RowCol{0, 3}, Block::State::REST);
	pit->spawn_block(Block::Color::RED, RowCol{0, 4}, Block::State::REST);
	pit->spawn_block(Block::Color::RED, RowCol{-1, 2}, Block::State::REST);

	block_director->swap(RowCol{0,2});

	run_game_ticks(Block::SWAP_TIME);
	EXPECT_EQ(3, counter->last_match.combo);
	EXPECT_FALSE(counter->last_match.chaining);

	const int FALL1_TIME = static_cast<int>(std::ceil(static_cast<float>(BLOCK_H)/FALL_SPEED));
	run_game_ticks(Block::BREAK_TIME + FALL1_TIME);
	EXPECT_EQ(3, counter->last_match.combo);
	EXPECT_TRUE(counter->last_match.chaining);
}

/**
 * Tests whether a chain event is correctly generated by the BlockDirector.
 */
TEST_F(GameEventTest, Chain)
{
	pit->spawn_block(Block::Color::BLUE, RowCol{0, 0}, Block::State::REST);
	pit->spawn_block(Block::Color::BLUE, RowCol{0, 1}, Block::State::REST);
	pit->spawn_block(Block::Color::RED, RowCol{0, 2}, Block::State::REST);
	pit->spawn_block(Block::Color::BLUE, RowCol{0, 3}, Block::State::REST);
	pit->spawn_block(Block::Color::RED, RowCol{0, 4}, Block::State::REST);
	pit->spawn_block(Block::Color::RED, RowCol{-1, 2}, Block::State::REST);

	block_director->swap(RowCol{0,2});

	const int FALL1_TIME = static_cast<int>(std::ceil(static_cast<float>(BLOCK_H)/FALL_SPEED));
	run_game_ticks(Block::SWAP_TIME + Block::BREAK_TIME + FALL1_TIME + Block::BREAK_TIME);
	EXPECT_EQ(1, counter->last_chain.counter);
}

/**
 * Tests whether a block dies event is correctly generated by the BlockDirector.
 */
TEST_F(GameEventTest, BlockDies)
{
	pit->spawn_block(Block::Color::BLUE, RowCol{0, 0}, Block::State::BREAK);
	run_game_ticks(Block::BREAK_TIME);
	EXPECT_EQ(1, counter->countBlockDies);

	pit->spawn_block(Block::Color::FAKE, RowCol{0, 0}, Block::State::BREAK);
	run_game_ticks(Block::BREAK_TIME);
	EXPECT_EQ(1, counter->countBlockDies);
}

/**
 * Tests whether a garbage dissolve event is correctly generated by the BlockDirector.
 */
TEST_F(GameEventTest, GarbageDissolves)
{
	pit->spawn_block(Block::Color::BLUE, RowCol{0, 0}, Block::State::REST);
	pit->spawn_block(Block::Color::BLUE, RowCol{0, 1}, Block::State::REST);
	pit->spawn_block(Block::Color::BLUE, RowCol{0, 3}, Block::State::REST);
	pit->spawn_garbage(RowCol{-1, 2}, 3, 1);

	block_director->swap(RowCol{0,2});
	run_game_ticks(Block::SWAP_TIME + Garbage::DISSOLVE_TIME);
	EXPECT_EQ(1, counter->countGarbageDissolves);
}
