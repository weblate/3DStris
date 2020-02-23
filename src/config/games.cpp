#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/writer.h>
#include <3dstris/config/games.hpp>
#include <3dstris/util/fs.hpp>
#include <3dstris/util/log.hpp>
#include <algorithm>

#define MEMBER_CHECK_TYPE(type, check_type, value)                        \
	type value;                                                           \
	{                                                                     \
		const auto _##value = mpack_node_map_cstr_optional(game, #value); \
		if (mpack_node_type(_##value) != mpack_type_##check_type) {       \
			LOG_WARN("Found invalid game at index %u, discarding", i);    \
			continue;                                                     \
		}                                                                 \
		value = mpack_node_##type(_##value);                              \
	}
#define MEMBER(type, value) MEMBER_CHECK_TYPE(type, type, value)

#define SERIALIZE_MEMBER(type, value)  \
	mpack_write_cstr(&writer, #value); \
	mpack_write_##type(&writer, game.value);

static bool validateJson(const rapidjson::Document& doc) {
	return !doc.HasParseError() && doc.IsArray();
}

static bool validateGame(
	const rapidjson::GenericValue<rapidjson::UTF8<char>>& game) {
	return game.FindMember("time") != game.MemberEnd() &&
		   game.FindMember("date") != game.MemberEnd() &&
		   game.FindMember("pps") != game.MemberEnd();
}

Games::Games() {
	static constexpr auto GAMES_JSON_PATH = "sdmc:/3ds/3dstris/games.json";

	LOG_INFO("Loading games");

	if (exists(GAMES_JSON_PATH) && !exists(GAMES_PATH)) {
		LOG_INFO(
			"Found games.json but not games.mp; loading from JSON, saving as "
			"MP");

		FILE* file = fopen(GAMES_JSON_PATH, "r");

		char readBuffer[1024];
		rapidjson::FileReadStream fileStream(file, readBuffer,
											 sizeof readBuffer);

		rapidjson::Document document;
		document.ParseStream(fileStream);

		fclose(file);

		if (!validateJson(document)) {
			LOG_ERROR("Failed to load games");
			save();
			_failed = true;

			return;
		}
		LOG_DEBUG("Reserving space for %u games", document.Size());
		games.reserve(document.Size());

		for (const auto& object : document.GetArray()) {
			if (validateGame(object)) {
				games.push_back({object["date"].GetInt64(),   //
								 object["time"].GetDouble(),  //
								 object["pps"].GetDouble()});
			}
		}

		std::sort(games.begin(), games.end(), std::less<SavedGame>());
		save();

		return;
	} else if (!exists(GAMES_PATH)) {
		LOG_INFO("Creating games file");
		save();

		return;
	}

	mpack_tree_t tree;
	mpack_tree_init_filename(&tree, GAMES_PATH, 0);
	mpack_tree_parse(&tree);
	mpack_node_t root = mpack_tree_root(&tree);

	const size_t length = mpack_node_array_length(root);
	LOG_DEBUG("Reserving space for %u games", length);
	games.reserve(length);
	for (size_t i = 0; i < length; ++i) {
		const auto game = mpack_node_array_at(root, i);
		if (mpack_node_type(game) != mpack_type_map) {
			LOG_WARN("Found invalid game at index %u, discarding", i);
			continue;
		}

		using i64 = long long;
		MEMBER_CHECK_TYPE(i64, uint, date)
		MEMBER(double, time)
		MEMBER(double, pps)
		games.push_back({date, time, pps});
	}
	std::sort(games.begin(), games.end(), std::less<SavedGame>());

	if (mpack_tree_destroy(&tree) != mpack_ok) {
		LOG_ERROR("Failed to decode games");
		save();
		_failed = true;
	} else {
		LOG_INFO("Loaded games");
	}
}

void Games::serialize(mpack_writer_t& writer) const {
	mpack_start_array(&writer, games.size());

	for (const auto& game : games) {
		mpack_start_map(&writer, 3);

		SERIALIZE_MEMBER(double, time)
		SERIALIZE_MEMBER(i64, date)
		SERIALIZE_MEMBER(double, pps)

		mpack_finish_map(&writer);
	}

	mpack_finish_array(&writer);
}

const SavedGames& Games::all() const noexcept {
	return games;
}

void Games::push(SavedGame&& game) {
	games.push_back(game);
	std::sort(games.begin(), games.end(), std::less<SavedGame>());
}

void Games::save() {
	LOG_INFO("Saving games");

	s32 mainPrio;
	svcGetThreadPriority(&mainPrio, CUR_THREAD_HANDLE);

	threadCreate(
		[](void* games) {
			char* data;
			size_t size;
			mpack_writer_t writer;
			mpack_writer_init_growable(&writer, &data, &size);

			static_cast<Games*>(games)->serialize(writer);

			if (mpack_writer_destroy(&writer) != mpack_ok) {
				LOG_ERROR("Failed to encode games");
				return;
			}

			FILE* file = fopen(GAMES_PATH, "w");
			fwrite(data, sizeof(char), size, file);
			fclose(file);

			delete[] data;

			LOG_INFO("Saved games");
		},
		this, 2048, mainPrio + 1, -2, true);
}

bool Games::failed() const noexcept {
	return _failed;
}
