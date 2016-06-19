/**
 * context.hpp
 * The context interfaces in this module are abstractions of SDL functions.
 * This module is independent of the concrete library-specific implementation.
 */
#pragma once

#include "globals.hpp"
#include <memory>

/**
 * Represents an environment for drawing stuff to the screen. Implemented with SDL.
 */
class IVideoContext
{
public:
	virtual void drawGfx(Gfx gfx, Point loc) const =0;
};

/**
 * Basic interface that specifies that an object can be drawn to the screen.
 */
class IScreenObject
{
	public: virtual void draw(const IVideoContext& context, float dt) =0; // dt: fraction of current display frame time elapsed
};

/**
 * Basic interface for animated objects
 */
class IAnimation : public IScreenObject
{
	public: virtual void animate() =0; // Called once per frame to update animation
};

/**
 * Basic interface for objects subject to game logic
 */
class ILogicObject
{
	public: virtual void update() =0; // advance the object by one tick
};

class IHistoryObject {}; // interface go-back etc.

class Block;
using SharedBlock = std::shared_ptr<Block>;
using WeakBlock = std::weak_ptr<Block>;

/**
 * This subscriber receives notifications about game-logic events,
 * such as blocks arriving at a certain state.
 */
class ILogicSubscriber
{
public:
	/**
	 * The block has fallen down far enough to arrive at the center of a new row.
	 */
	virtual void notify_block_arrive_row(WeakBlock block) =0;
	virtual void notify_block_dead(WeakBlock block) =0;
};

/**
 * The pit does not own its contained blocks (the stage does), but it remembers where blocks are and which
 * spaces are free or blocked.
 */
class IPit
{
public:
	virtual Point loc() const =0; // get location of pit on canvas
	virtual void block(RowCol rc, WeakBlock block) =0;
	virtual void unblock(RowCol rc) =0;
	virtual WeakBlock block_at(RowCol rc) const =0; // true if the location in the pit is occupied, e.g. by a non-falling block
};

using SharedAnimation = std::shared_ptr<IAnimation>;
using SharedLogic = std::shared_ptr<ILogicObject>;
using SharedSubscriber = std::shared_ptr<ILogicSubscriber>;
using WeakSubscriber = std::weak_ptr<ILogicSubscriber>;
using SharedPit = std::shared_ptr<IPit>;
using WeakPit = std::weak_ptr<IPit>;
