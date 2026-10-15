#include "achievements.h"
#include "achievement_memory.h"
#include "common_rtl.h"
#include "rc_client.h"
#include "rc_error.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __ANDROID__
#include "desktop/sdl_compat.h"
#include <jni.h>
#endif

/* rc_client, including HTTP completions, runs only on the simulation thread.
 * UI/HTTP threads exchange owned data through this small queue. No credentials
 * are persisted or logged, and no client/network exists until explicit login. */
static pthread_mutex_t s_lock = PTHREAD_MUTEX_INITIALIZER;
static rc_client_t *s_client;
static char s_status[256] = "Offline — optional RetroAchievements login (softcore)";
static char *s_username, *s_password;
static int s_action, s_reset;
static unsigned s_next_id;
static int s_loaded;
typedef struct Request {
    struct Request *next;
    unsigned id;
    rc_client_server_callback_t callback;
    void *context;
    char *body;
    size_t size;
    int code, ready;
} Request;
static Request *s_requests;
#ifdef __ANDROID__
static jclass s_java;
static jmethodID s_dispatch, s_notice;
#endif

static void ClearSecret(char *value) {
    if (!value) return;
    volatile char *p = value;
    size_t n = strlen(value);
    while (n--) *p++ = 0;
    free(value);
}
static void Status(const char *message) {
    pthread_mutex_lock(&s_lock);
    snprintf(s_status, sizeof(s_status), "%s", message);
    pthread_mutex_unlock(&s_lock);
}
static uint32_t ReadMemory(uint32_t address, uint8_t *buffer, uint32_t size,
                           rc_client_t *client) {
    (void)client;
    return (uint32_t)yoshi_achievement_read(address, buffer, size, g_ram,
        g_sram, g_sram_size > 0 ? (size_t)g_sram_size : 0);
}
static void Notify(const char *message) {
#ifdef __ANDROID__
    JNIEnv *env = SDL_GetAndroidJNIEnv();
    if (env && s_java) {
        jstring text = (*env)->NewStringUTF(env, message);
        if (text) { (*env)->CallStaticVoidMethod(env, s_java, s_notice, text);
                    (*env)->DeleteLocalRef(env, text); }
        if ((*env)->ExceptionCheck(env)) (*env)->ExceptionClear(env);
    }
#else
    (void)message;
#endif
}
static void ServerCall(const rc_api_request_t *api,
    rc_client_server_callback_t callback, void *context, rc_client_t *client) {
    (void)client;
    Request *request = calloc(1, sizeof(*request));
    if (!request) {
        rc_api_server_response_t response = {"", 0, RC_API_SERVER_RESPONSE_CLIENT_ERROR};
        callback(&response, context);
        return;
    }
    request->callback = callback;
    request->context = context;
    pthread_mutex_lock(&s_lock);
    request->id = ++s_next_id;
    request->next = s_requests;
    s_requests = request;
    pthread_mutex_unlock(&s_lock);
#ifdef __ANDROID__
    JNIEnv *env = SDL_GetAndroidJNIEnv();
    if (env && s_java) {
        jstring url = (*env)->NewStringUTF(env, api->url);
        jstring post = api->post_data ? (*env)->NewStringUTF(env, api->post_data) : NULL;
        jstring type = (*env)->NewStringUTF(env, api->content_type ? api->content_type : "application/x-www-form-urlencoded");
        if (url && type) (*env)->CallStaticVoidMethod(env, s_java, s_dispatch,
            (jlong)request->id, url, post, type);
        int failed = (*env)->ExceptionCheck(env) || !url || !type;
        if (failed) (*env)->ExceptionClear(env);
        if (url) (*env)->DeleteLocalRef(env, url);
        if (post) (*env)->DeleteLocalRef(env, post);
        if (type) (*env)->DeleteLocalRef(env, type);
        if (!failed) return;
    }
#else
    (void)api;
#endif
    pthread_mutex_lock(&s_lock);
    request->code = RC_API_SERVER_RESPONSE_CLIENT_ERROR;
    request->ready = 1;
    pthread_mutex_unlock(&s_lock);
}
static void Summary(void) {
    rc_client_user_game_summary_t summary;
    rc_client_get_user_game_summary(s_client, &summary);
    char message[256];
    snprintf(message, sizeof(message), "Yoshi's Island (558) — softcore: %u / %u unlocked",
             summary.num_unlocked_achievements, summary.num_core_achievements);
    Status(message);
}
static void Loaded(int result, const char *error, rc_client_t *client, void *context) {
    (void)error; (void)context;
    const rc_client_game_t *game = rc_client_get_game_info(client);
    if (result != RC_OK || !game || game->id != 558) {
        rc_client_unload_game(client);
        Status("This USA v1.1 hash is not available for game 558, or the service is offline. Gameplay continues.");
        Notify("Achievements unavailable for this game/session — playing offline");
        return;
    }
    s_loaded = 1;
    Summary();
    Notify("RetroAchievements connected — softcore");
}
static void LoggedIn(int result, const char *error, rc_client_t *client, void *context) {
    (void)error; (void)context;
    if (result != RC_OK) {
        Status("Login failed or service unavailable. Check your credentials or try later. Gameplay continues offline.");
        Notify("RetroAchievements login failed or service unavailable — playing offline");
        return;
    }
    Status("Loading game 558 achievements (softcore)…");
    rc_client_begin_load_game(client, "ce1e3e33b6e39d37b43d7de599f9e785", Loaded, NULL);
}
static void Event(const rc_client_event_t *event, rc_client_t *client) {
    (void)client;
    switch (event->type) {
    case RC_CLIENT_EVENT_ACHIEVEMENT_TRIGGERED:
        if (event->achievement) Notify(event->achievement->title);
        Summary();
        break;
    case RC_CLIENT_EVENT_DISCONNECTED:
        Status("Offline — achievement submissions queued for this session. Keep playing.");
        break;
    case RC_CLIENT_EVENT_RECONNECTED: Summary(); break;
    case RC_CLIENT_EVENT_SERVER_ERROR:
        Status("Achievement service error. Gameplay continues; reconnect from the menu.");
        break;
    default: break;
    }
}
void YoshiAchievementsReset(void) {
    pthread_mutex_lock(&s_lock); s_reset = 1; pthread_mutex_unlock(&s_lock);
}
void YoshiAchievementsBegin(void) {
    pthread_mutex_lock(&s_lock);
    int action = s_action, reset = s_reset;
    char *user = s_username, *password = s_password;
    s_action = s_reset = 0; s_username = s_password = NULL;
    pthread_mutex_unlock(&s_lock);
    if (action) {
        if (!s_client && action == 1) {
            s_client = rc_client_create(ReadMemory, ServerCall);
            if (s_client) {
                rc_client_set_hardcore_enabled(s_client, 0);
                rc_client_set_event_handler(s_client, Event);
            }
        }
        s_loaded = 0;
        if (s_client) rc_client_logout(s_client);
        if (action == 1 && s_client) {
            Status("Connecting to RetroAchievements (softcore)…");
            rc_client_begin_login_with_password(s_client, user, password, LoggedIn, NULL);
        } else Status(action == 1 ? "Achievements unavailable; gameplay continues offline" : "Offline — signed out");
    }
    free(user); ClearSecret(password);
    if (reset && s_client) rc_client_reset(s_client);
    /* Detach each ready response before invoking callbacks: callbacks may
     * synchronously add another request. Never hold the queue mutex across RA. */
    for (;;) {
        pthread_mutex_lock(&s_lock);
        Request **link = &s_requests;
        while (*link && !(*link)->ready) link = &(*link)->next;
        Request *request = *link;
        if (request) *link = request->next;
        pthread_mutex_unlock(&s_lock);
        if (!request) break;
        rc_api_server_response_t response = {request->body ? request->body : "",
                                             request->size, request->code};
        request->callback(&response, request->context);
        ClearSecret(request->body); free(request);
    }
}
void YoshiAchievementsEnd(void) {
    if (s_client) rc_client_do_frame(s_client);
}
#ifdef __ANDROID__
JNIEXPORT void JNICALL Java_com_hector_yoshi_YoshiAchievements_initialize(JNIEnv *env, jclass type) {
    if (!s_java) {
        s_java = (*env)->NewGlobalRef(env, type);
        s_dispatch = (*env)->GetStaticMethodID(env, type, "dispatch", "(JLjava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
        s_notice = (*env)->GetStaticMethodID(env, type, "notice", "(Ljava/lang/String;)V");
    }
}
JNIEXPORT jstring JNICALL Java_com_hector_yoshi_YoshiAchievements_status(JNIEnv *env, jclass type) {
    (void)type;
    pthread_mutex_lock(&s_lock);
    jstring result = (*env)->NewStringUTF(env, s_status);
    pthread_mutex_unlock(&s_lock);
    return result;
}
JNIEXPORT void JNICALL Java_com_hector_yoshi_YoshiAchievements_login(JNIEnv *env, jclass type, jstring user, jstring password) {
    (void)type;
    const char *u = (*env)->GetStringUTFChars(env, user, NULL);
    const char *p = (*env)->GetStringUTFChars(env, password, NULL);
    if (u && p) {
        char *name = strdup(u), *secret = strdup(p);
        if (name && secret) {
            pthread_mutex_lock(&s_lock);
            free(s_username); ClearSecret(s_password);
            s_username = name; s_password = secret; s_action = 1;
            pthread_mutex_unlock(&s_lock);
        } else { free(name); ClearSecret(secret); }
    }
    if (u) (*env)->ReleaseStringUTFChars(env, user, u);
    if (p) (*env)->ReleaseStringUTFChars(env, password, p);
}
JNIEXPORT void JNICALL Java_com_hector_yoshi_YoshiAchievements_logout(JNIEnv *env, jclass type) {
    (void)env; (void)type;
    pthread_mutex_lock(&s_lock);
    free(s_username); ClearSecret(s_password); s_username = s_password = NULL;
    s_action = 2;
    pthread_mutex_unlock(&s_lock);
}
JNIEXPORT void JNICALL Java_com_hector_yoshi_YoshiAchievements_reply(JNIEnv *env, jclass type, jlong id, jint code, jbyteArray body) {
    (void)type;
    jsize size = body ? (*env)->GetArrayLength(env, body) : 0;
    if (size > 4 * 1024 * 1024) size = 0;
    char *copy = calloc((size_t)size + 1, 1);
    if (copy && size) (*env)->GetByteArrayRegion(env, body, 0, size, (jbyte*)copy);
    pthread_mutex_lock(&s_lock);
    for (Request *r = s_requests; r; r = r->next) if (r->id == (unsigned)id && !r->ready) {
        r->body = copy; copy = NULL; r->size = r->body ? (size_t)size : 0;
        r->code = r->body ? code : RC_API_SERVER_RESPONSE_CLIENT_ERROR;
        r->ready = 1;
        break;
    }
    pthread_mutex_unlock(&s_lock);
    ClearSecret(copy);
}
#endif
