#pragma once

#include <3ds.h>
#include <mpack/mpack.h>
#include <vector>

struct SavedGame {
	double time;
	double pps;
	time_t date;
	u16 lines;

	void dateString(char* buf, size_t size,
					const char* __restrict format = "%F %T") const {
		strftime(buf, size, format, localtime(&date));
	}

	bool operator<(const SavedGame& b) const {
		return lines < b.lines || time < b.time;
	}
};

using SavedGames = std::vector<SavedGame>;

class Games {
   public:
	Games();

	void serialize(mpack_writer_t& writer) const;

	const SavedGames& all() const noexcept;

	void save();
	void push(SavedGame&& game);

	bool failed() const noexcept;

   private:
	static constexpr auto GAMES_PATH = "sdmc:/3ds/3dstris/games.mp";

	SavedGames games;

	bool _failed = false;
};
