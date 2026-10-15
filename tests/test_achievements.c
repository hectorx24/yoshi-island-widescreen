/* Synthetic API fixtures only: tests the real native rc_client integration,
 * never authenticates a user or awards a real RetroAchievements achievement. */
#include <assert.h>
#include "../src/achievements.c"
uint8_t g_ram[0x20000];
static uint8_t cartridge[0x8000];
uint8_t *g_sram = cartridge;
int g_sram_size = sizeof(cartridge);
static int calls, awards, wrong_game;
static void MockServer(const rc_api_request_t *request,
    rc_client_server_callback_t callback, void *context, rc_client_t *client) {
    assert(!rc_client_get_hardcore_enabled(client));
    const char *body = "{\"Success\":true}";
    ++calls;
    if (strstr(request->post_data, "r=login2")) {
        body = "{\"Success\":true,\"User\":\"Test\",\"Token\":\"SyntheticToken\",\"Score\":0,\"SoftcoreScore\":0,\"Messages\":0}";
    } else if (strstr(request->post_data, "r=achievementsets")) {
        assert(strstr(request->post_data, "ce1e3e33b6e39d37b43d7de599f9e785"));
        body = wrong_game ? "{\"Success\":true,\"GameID\":1234}" :
          "{\"Success\":true,\"GameId\":558,\"Title\":\"Synthetic fixture\",\"ConsoleId\":3,"
          "\"ImageIconUrl\":\"https://retroachievements.org/Images/00000.png\","
          "\"RichPresenceGameId\":558,\"RichPresencePatch\":\"\",\"Sets\":[{"
          "\"AchievementSetId\":558,\"GameId\":558,\"Title\":null,\"Type\":\"core\","
          "\"ImageIconUrl\":\"https://retroachievements.org/Images/00000.png\","
          "\"Achievements\":[{\"ID\":1,\"Title\":\"Synthetic RAM check\",\"Description\":\"Fixture\","
          "\"Flags\":3,\"Points\":5,\"MemAddr\":\"0xH0001=3_0xH020000=7\","
          "\"Author\":\"Test\",\"BadgeName\":\"00000\",\"Created\":1,\"Modified\":1}],\"Leaderboards\":[]}]}";
    } else if (strstr(request->post_data, "r=startsession")) {
        body = "{\"Success\":true,\"Unlocks\":[],\"HardcoreUnlocks\":[],\"ServerNow\":1}";
    } else if (strstr(request->post_data, "r=awardachievement")) {
        assert(strstr(request->post_data, "h=0"));
        ++awards;
    }
    rc_api_server_response_t response = {body, strlen(body), 200};
    callback(&response, context);
}
int main(void) {
    for (int i = 0; i < 100; ++i) { YoshiAchievementsBegin(); YoshiAchievementsEnd(); }
    assert(!s_client && !s_requests); /* zero network/client before opt-in */
    s_username = strdup("Synthetic"); s_password = strdup("NeverReal"); s_action = 1;
    YoshiAchievementsBegin();
    assert(s_client && !rc_client_get_hardcore_enabled(s_client));
    assert(strstr(s_status, "Login failed"));
    assert(!s_password && !s_username && !s_requests);
    rc_client_destroy(s_client);
    s_client = rc_client_create(ReadMemory, MockServer);
    rc_client_set_hardcore_enabled(s_client, 0);
    rc_client_set_event_handler(s_client, Event);
    rc_client_begin_login_with_password(s_client, "Test", "Fixture", LoggedIn, NULL);
    if (!s_loaded) fprintf(stderr, "Fixture status: %s\n", s_status);
    assert(s_loaded && strstr(s_status, "0 / 1"));
    YoshiAchievementsEnd(); /* false condition arms achievement */
    g_ram[1] = 3; cartridge[0] = 7;
    YoshiAchievementsEnd();
    assert(strstr(s_status, "1 / 1") && awards == 1);
    YoshiAchievementsReset(); YoshiAchievementsBegin();
    assert(!rc_client_get_hardcore_enabled(s_client));
    s_action = 2; YoshiAchievementsBegin();
    assert(!rc_client_get_user_info(s_client) && !s_loaded);
    wrong_game = 1;
    rc_client_begin_login_with_password(s_client, "Test", "Fixture", LoggedIn, NULL);
    assert(!s_loaded && !rc_client_is_game_loaded(s_client));
    rc_client_destroy(s_client); s_client = NULL;
    printf("Native rc_client: opt-in/offline, softcore, exact v1.1 hash, RAM evaluation/unlock, reset/logout, unsupported game verified (%d fixture requests)\n", calls);
}
