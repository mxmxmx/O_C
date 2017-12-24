#include "gtest/gtest.h"
#include "util/util_settings.h"

class TestU8Settings : public settings::SettingsBase<TestU8Settings, 1> { };
SETTINGS_DECLARE(TestU8Settings, 1) {
  { 0, 0, 8, "U8", nullptr, settings::STORAGE_TYPE_U8 },
};

TEST(TestSettings,TestU8)
{
  EXPECT_EQ(1U, TestU8Settings::storageSize());

  TestU8Settings settings;
  settings.InitDefaults();
  EXPECT_EQ(0, settings.get_value(0));
}

class TestPackU4EvenSettings : public settings::SettingsBase<TestPackU4EvenSettings, 3> { };
SETTINGS_DECLARE(TestPackU4EvenSettings, 3) {
  { 0, 0, 15, "U4", nullptr, settings::STORAGE_TYPE_U4 },
  { 0, 0, 15, "U4", nullptr, settings::STORAGE_TYPE_U4 },
  { 0, -1, 8, "I32", nullptr, settings::STORAGE_TYPE_I32 },
};

TEST(TestSettings,TestPackU4Even)
{
  EXPECT_EQ(5, TestPackU4EvenSettings::storageSize());

  TestPackU4EvenSettings settings;
  settings.InitDefaults();
  EXPECT_TRUE(settings.apply_value(0, 0x09)); // 1001
  EXPECT_TRUE(settings.apply_value(1, 0x06)); // 0110
  EXPECT_TRUE(settings.apply_value(2, -1));

  std::vector<uint8_t> data;
  data.resize(TestPackU4EvenSettings::storageSize() + 1, 0xff);

  size_t saved_size = settings.Save(&data.front());
  EXPECT_EQ(TestPackU4EvenSettings::storageSize(), saved_size);
  EXPECT_EQ(0xff, data[TestPackU4EvenSettings::storageSize()]);

  settings.InitDefaults();
  size_t restored_size = settings.Restore(&data.front());
  EXPECT_EQ(saved_size, restored_size);

  EXPECT_EQ(0x09, settings.get_value(0));
  EXPECT_EQ(0x06, settings.get_value(1));
  EXPECT_EQ(-1, settings.get_value(2));
}

class TestPackU4OddSettings : public settings::SettingsBase<TestPackU4OddSettings, 2> { };
SETTINGS_DECLARE(TestPackU4OddSettings, 2) {
  { 0, 0, 15, "U4", nullptr, settings::STORAGE_TYPE_U4 },
  { 0, -1, 8, "I32", nullptr, settings::STORAGE_TYPE_I32 },
};

TEST(TestSettings,TestPackU4Odd)
{
  EXPECT_EQ(5, TestPackU4OddSettings::storageSize());

  TestPackU4OddSettings settings;
  settings.InitDefaults();
  EXPECT_TRUE(settings.apply_value(0, 0x09)); // 1001
  EXPECT_TRUE(settings.apply_value(1, -1));

  std::vector<uint8_t> data;
  data.resize(TestPackU4OddSettings::storageSize() + 1, 0xff);

  size_t saved_size = settings.Save(&data.front());
  EXPECT_EQ(TestPackU4OddSettings::storageSize(), saved_size);
  EXPECT_EQ(0xff, data[TestPackU4OddSettings::storageSize()]);

  settings.InitDefaults();
  size_t restored_size = settings.Restore(&data.front());
  EXPECT_EQ(saved_size, restored_size);

  EXPECT_EQ(0x09, settings.get_value(0));
  EXPECT_EQ(-1, settings.get_value(1));
}

class TestPackU4OddEndSettings : public settings::SettingsBase<TestPackU4OddEndSettings, 2> { };
SETTINGS_DECLARE(TestPackU4OddEndSettings, 2) {
  { 0, -1, 8, "I32", nullptr, settings::STORAGE_TYPE_I32 },
  { 0, 0, 15, "U4", nullptr, settings::STORAGE_TYPE_U4 },
};

TEST(TestSettings,TestPackU4OddEnd)
{
  EXPECT_EQ(5, TestPackU4OddSettings::storageSize());

  TestPackU4OddEndSettings settings;
  settings.InitDefaults();
  EXPECT_TRUE(settings.apply_value(0, -1));
  EXPECT_TRUE(settings.apply_value(1, 0x09));

  std::vector<uint8_t> data;
  data.resize(TestPackU4OddEndSettings::storageSize() + 1, 0xff);

  size_t saved_size = settings.Save(&data.front());
  EXPECT_EQ(TestPackU4OddEndSettings::storageSize(), saved_size);
  EXPECT_EQ(0xff, data[TestPackU4OddEndSettings::storageSize()]);

  settings.InitDefaults();
  size_t restored_size = settings.Restore(&data.front());
  EXPECT_EQ(saved_size, restored_size);

  EXPECT_EQ(-1, settings.get_value(0));
  EXPECT_EQ(0x09, settings.get_value(1));
}
