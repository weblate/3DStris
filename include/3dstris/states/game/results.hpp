#pragma once

#include <3dstris/gui/widgets/button.hpp>
#include <3dstris/state.hpp>
#include <3dstris/states/game/ingame.hpp>

class Results final : public State {
   public:
	explicit Results(Ingame* parent);

	void update(double dt) override;
	void draw(bool bottom) const override;

   protected:
	static constexpr Color RESULTS = C2D_Color32(0, 0, 0, 190);

	Ingame* parent;

	Text dead;

	GUI gui;

	Button& restart;
	Button& menu;
};
