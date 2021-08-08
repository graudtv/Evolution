#include "TiledGameModel.h"
#include "IGameObject.h"
#include "ICollisionResolver.h"
#include "IGameFinalizer.h"
#include <cassert>

namespace evo {
namespace snake {

TiledGameModel::TiledGameModel(size_t height, size_t width,
		ICollisionResolver *resolver, IGameFinalizer *finalizer) :
	m_collision_resolver(*resolver),
	m_finalizer(*finalizer),
	m_map(height, width)
{
	assert(resolver != nullptr && "resolver must be a valid pointer");
	assert(finalizer != nullptr && "finalizer must be a valid pointer");
}

/* If initial_location overlays some previously added object location,
 * when BadLocationError is thrown, and TiledGameModel is in
 * unspecified state (base exception guarantee), game object
 * remains unchanged */
void TiledGameModel::add_object(IGameObject *object, const CoordArray& initial_location)
{
	assert(object != nullptr && "object must be a valid pointer");
	assert(object->game_model() == nullptr && "object is already attached to some game");

	std::for_each(initial_location.begin(), initial_location.end(),
		[this, object](Coord coord) {
			auto& tile = m_map[coord.row][coord.column];
			if (tile) {
				throw BadLocationError("TiledGameModel::add_object: "
					"trying to place new object on already occupied position");
			}
			tile = object;
		}
	);

	object->m_game_model = this;
	notify_on_object_attachment(*this, *object);
}

void TiledGameModel::kill_object(IGameObject& object)
{
	assert(object.game_model() == this && "object is not attached to this game model");

	// it could be refactored, but I'm not sure it will
	for (size_t row = 0; row < m_map.height(); ++row)
		for (size_t column = 0; column < m_map.width(); ++column) {
			auto& tile = m_map[row][column];
			assert(tile == &object);
			tile = nullptr;
		}
	m_objects.erase(
		std::find(m_objects.begin(), m_objects.end(), &object)
	);
	object.m_game_model = nullptr;
	notify_on_object_kill(*this, object);
}

void TiledGameModel::start_game()
{
	assert(m_is_model_running == false && "game is already running");
	m_is_model_running = true;

	notify_on_game_start(*this);

	while (m_is_model_running) {
		move_game_objects();
		notify_on_all_objects_moved(*this);
		m_collision_resolver.resolve_collisions(*this);
		notify_on_collisions_resolved(*this);
		m_is_model_running = !m_finalizer.is_game_finished(*this);
	}

	notify_on_game_finish(*this);
}

void TiledGameModel::move_game_objects()
{
	std::for_each(m_objects.begin(), m_objects.end(),
		[](IGameObject *object) { object->make_move(); }
	);	
}

} // snake namespace end
} // evo namespace end