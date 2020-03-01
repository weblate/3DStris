#include <3dstris/states/game/ingame.hpp>
#include <3dstris/states/game/paused.hpp>
#include <algorithm>

static std::array<PieceType, 7> genBag(std::mt19937_64& rng) {
	std::array<PieceType, 7> pieces{I, O, L, J, S, T, Z};

	std::shuffle(pieces.begin(), pieces.end(), rng);
	return pieces;
}

Ingame::Ingame()
	: State(),
	  board(10, 20),
	  tileSize((SCREEN_HEIGHT - 10) / board.height),
	  bagRNG(osGetTime()),
	  upcoming(5),
	  piece(board, I) {
	origin = {(SCREEN_WIDTH - board.width * tileSize) / 2.0f, 10};
	reset();
}

void Ingame::reset() {
	board.reset();

	const auto _bag = genBag(bagRNG);
	bag.insert(bag.end(), std::make_move_iterator(_bag.begin()),
			   std::make_move_iterator(_bag.end()));

	piece.reset(bag.front());
	bag.pop_front();

	hold = NONE;
	hasHeld = false;
}

void Ingame::update(const double dt) {
	const u32 kDown = hidKeysDown();
	const u32 kHeld = hidKeysHeld();

	if (kDown & KEY_START) {
		game.pushState(make_unique<Paused>(this));
		return;
	}

	piece.update(dt, kDown, kHeld);

	if (piece.hasSet()) {
		hasHeld = false;
		piece.reset(bag.front());
		bag.pop_front();
		if (bag.size() < upcoming) {
			for (const auto& p : genBag(bagRNG)) {
				bag.push_back(std::move(p));
			}
		}
	}

	if (!hasHeld && game.isPressed(kDown, Keybinds::HOLD)) {
		hasHeld = true;
		if (hold == NONE && piece.getType() != NONE &&
			piece.getType() != INVALID) {
			hold = piece.getType();
			piece.reset(bag.front());
			bag.pop_front();
		} else {
			const auto tmp = piece.getType();
			piece.reset(hold);
			hold = tmp;
		}
	}
}

void Ingame::draw(const bool bottom) {
	if (bottom) {
		return;
	}
	C2D_TargetClear(game.getTop(), BACKGROUND);

	board.draw(origin, tileSize);
	piece.draw(origin, tileSize);

	// draw bag
	u32 y = 1;
	for (u32 i = 0; i < upcoming; ++i) {
		const PieceType& p = bag[i];

		if (p == I) {
			--y;
		}

		Piece::draw({origin.x + (board.width + 1 + (p == O)) * tileSize,
					 origin.y + y * tileSize},
					tileSize, Shapes::ALL[p], p);

		y += Shapes::ALL[p].size();
		if (p == O) {
			++y;
		}
	}

	// draw held piece
	if (hold != NONE && hold != INVALID) {
		Piece::draw({origin.x - (Shapes::ALL[hold].size() + 1) * tileSize,
					 origin.y + tileSize},
					tileSize, Shapes::ALL[hold], hold);
	}
}
