#include <unity.h>

#include <cstring>

#include "config.h"

namespace {

// A step that does nothing makes a sequence invalid (used below).
constexpr ActionStep EMPTY_STEPS[] = {{MOD_NONE, 0, nullptr, 0}};

void assert_page_layout(const PageDef &page) {
    TEST_ASSERT_NOT_NULL(page.title);
    TEST_ASSERT_TRUE(page.button_count > 0);
    TEST_ASSERT_TRUE(page.button_count <= MAX_BUTTONS_PER_PAGE);
    for (size_t i = 0; i < page.button_count; ++i) {
        TEST_ASSERT_NOT_NULL(page.buttons[i].label);
        TEST_ASSERT_NOT_NULL(page.buttons[i].icon);
    }
}

} // namespace

void test_pages_exist(void) {
    TEST_ASSERT_TRUE(PAGE_COUNT > 0);
    TEST_ASSERT_TRUE(PAGE_COUNT <= MAX_PAGES);
}

void test_page_titles_are_unique(void) {
    for (size_t i = 0; i < PAGE_COUNT; ++i) {
        for (size_t j = i + 1; j < PAGE_COUNT; ++j) {
            TEST_ASSERT_TRUE(std::strcmp(PAGES[i].title, PAGES[j].title) != 0);
        }
    }
}

void test_layout_rejects_invalid_pages(void) {
    constexpr PageDef empty_page{nullptr, 0, {}, 0};
    TEST_ASSERT_FALSE(layout_is_valid(empty_page));

    constexpr PageDef missing_title{{}, 0, {{"A", "B", Keys(MOD_CTRL, 'a')}}, 1};
    TEST_ASSERT_FALSE(layout_is_valid(missing_title));

    constexpr PageDef missing_label{"Test", 0, {{nullptr, "B", Keys(MOD_CTRL, 'a')}}, 1};
    TEST_ASSERT_FALSE(layout_is_valid(missing_label));

    constexpr PageDef missing_icon{"Test", 0, {{"A", nullptr, Keys(MOD_CTRL, 'a')}}, 1};
    TEST_ASSERT_FALSE(layout_is_valid(missing_icon));
}

void test_every_page_has_a_valid_layout(void) {
    for (const PageDef &page : PAGES) {
        assert_page_layout(page);
    }
}

void test_every_button_has_an_action(void) {
    for (const PageDef &page : PAGES) {
        for (size_t i = 0; i < page.button_count; ++i) {
            TEST_ASSERT_TRUE(action_is_valid(page.buttons[i].action));
        }
    }
}

void test_modifier_masks(void) {
    TEST_ASSERT_TRUE(is_valid_mod_mask(MOD_CTRL | MOD_SHIFT | MOD_ALT | MOD_GUI));
    TEST_ASSERT_FALSE(is_valid_mod_mask(0x10));
}

void test_chord_actions(void) {
    constexpr ButtonAction combo = Keys(static_cast<uint8_t>(MOD_CTRL | MOD_SHIFT), 'm');
    TEST_ASSERT_TRUE(action_has_key(combo));
    TEST_ASSERT_TRUE(action_is_valid(combo));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(MOD_CTRL | MOD_SHIFT), combo.mods);
    TEST_ASSERT_EQUAL_UINT8('m', combo.key);
    TEST_ASSERT_EQUAL_UINT8(0, combo.media0);
    TEST_ASSERT_EQUAL_UINT8(0, combo.media1);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ActionKind::CHORD), static_cast<int>(combo.kind));

    // Modifier-only actions are valid (e.g. the Start button = Win key tap).
    constexpr ButtonAction modifier_only = Keys(MOD_GUI, 0);
    TEST_ASSERT_TRUE(action_is_valid(modifier_only));

    // A chord with neither a modifier nor a key sends nothing.
    constexpr ButtonAction empty = Keys(MOD_NONE, 0);
    TEST_ASSERT_FALSE(action_is_valid(empty));
}

void test_media_actions(void) {
    constexpr ButtonAction media = Media(MediaCode::PLAY_PAUSE);
    TEST_ASSERT_TRUE(action_is_valid(media));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ActionKind::MEDIA), static_cast<int>(media.kind));
    TEST_ASSERT_TRUE(media.media0 != 0 || media.media1 != 0);
    TEST_ASSERT_EQUAL_UINT8(8, media.media0); // KEY_MEDIA_PLAY_PAUSE bit
    TEST_ASSERT_EQUAL_UINT8(0, media.media1);

    constexpr ButtonAction volume = Media(MediaCode::VOL_UP);
    TEST_ASSERT_TRUE(action_is_valid(volume));
    TEST_ASSERT_EQUAL_UINT8(32, volume.media0);
}

void test_sequence_actions(void) {
    TEST_ASSERT_TRUE(action_is_valid(SEQUENCE_TERMINAL));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ActionKind::SEQUENCE),
                          static_cast<int>(SEQUENCE_TERMINAL.kind));
    TEST_ASSERT_EQUAL_UINT8(3, SEQUENCE_TERMINAL.step_count);
    TEST_ASSERT_NOT_NULL(SEQUENCE_TERMINAL.steps);

    // Step 1: Win+R with a pause for the Run dialog.
    TEST_ASSERT_EQUAL_UINT8(MOD_GUI, SEQUENCE_TERMINAL.steps[0].mods);
    TEST_ASSERT_EQUAL_UINT8('r', SEQUENCE_TERMINAL.steps[0].key);
    TEST_ASSERT_NULL(SEQUENCE_TERMINAL.steps[0].text);
    TEST_ASSERT_TRUE(SEQUENCE_TERMINAL.steps[0].pause_ms > 0);

    // Step 2: typed text.
    TEST_ASSERT_NOT_NULL(SEQUENCE_TERMINAL.steps[1].text);
    TEST_ASSERT_EQUAL_STRING("wt", SEQUENCE_TERMINAL.steps[1].text);

    // Step 3: Enter.
    TEST_ASSERT_TRUE(SEQUENCE_TERMINAL.steps[2].key != 0);

    constexpr ButtonAction invalid_sequence = Sequence(EMPTY_STEPS);
    TEST_ASSERT_FALSE(action_is_valid(invalid_sequence));
}

void test_invalid_actions_are_rejected(void) {
    constexpr ButtonAction none = {ActionKind::NONE, MOD_NONE, 0, 0, 0, nullptr, 0};
    TEST_ASSERT_FALSE(action_is_valid(none));

    constexpr ButtonAction bad_modifier = {ActionKind::CHORD, 0x10, 0, 0, 0, nullptr, 0};
    TEST_ASSERT_FALSE(action_is_valid(bad_modifier));

    constexpr ButtonAction bad_modifier_with_media = {ActionKind::CHORD, 0x10, 0, 8, 0, nullptr, 0};
    TEST_ASSERT_FALSE(action_is_valid(bad_modifier_with_media));

    constexpr ButtonAction empty_media = {ActionKind::MEDIA, MOD_NONE, 0, 0, 0, nullptr, 0};
    TEST_ASSERT_FALSE(action_is_valid(empty_media));
}

void test_page_text_is_present(void) {
    for (const PageDef &page : PAGES) {
        TEST_ASSERT_TRUE(std::strlen(page.title) > 0);
        for (size_t i = 0; i < page.button_count; ++i) {
            TEST_ASSERT_TRUE(std::strlen(page.buttons[i].label) > 0);
            TEST_ASSERT_TRUE(std::strlen(page.buttons[i].icon) > 0);
        }
    }
}

void test_page_accents_are_set(void) {
    for (const PageDef &page : PAGES) {
        TEST_ASSERT_TRUE(page.accent != 0);
    }
}

void test_config_is_consistent(void) {
    TEST_ASSERT_TRUE(config_is_valid());
}

void setup(void) {
    UNITY_BEGIN();
    RUN_TEST(test_pages_exist);
    RUN_TEST(test_page_titles_are_unique);
    RUN_TEST(test_layout_rejects_invalid_pages);
    RUN_TEST(test_every_page_has_a_valid_layout);
    RUN_TEST(test_every_button_has_an_action);
    RUN_TEST(test_modifier_masks);
    RUN_TEST(test_chord_actions);
    RUN_TEST(test_media_actions);
    RUN_TEST(test_sequence_actions);
    RUN_TEST(test_invalid_actions_are_rejected);
    RUN_TEST(test_page_text_is_present);
    RUN_TEST(test_page_accents_are_set);
    RUN_TEST(test_config_is_consistent);
    UNITY_END();
}

void loop(void) {}
