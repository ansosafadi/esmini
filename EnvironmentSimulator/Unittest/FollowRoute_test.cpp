#include <iostream>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <vector>
#include <chrono>

#include "pugixml.hpp"
#include "simple_expr.h"
#include "LaneIndependentRouter.hpp"

#define TRIG_ERR_MARGIN 0.001

using namespace roadmanager;
OpenDrive *odrSmall = nullptr;
OpenDrive *odrMedium = nullptr;
OpenDrive *odrLarge = nullptr;
class FollowRouteTest : public ::testing::Test
{
public:
    static void SetUpTestSuite()
    {
        // Position::GetOpenDrive()->LoadOpenDriveFile("../../../../esmini/resources/xodr/highway_example_with_merge_and_split.xodr");
        odrSmall = new OpenDrive("../../../../esmini/resources/xodr/highway_example_with_merge_and_split.xodr");

        // Position::GetOpenDrive()->LoadOpenDriveFile("../../../../esmini/resources/xodr/multi_intersections.xodr");
        odrMedium = new OpenDrive("../../../../esmini/resources/xodr/multi_intersections.xodr");

        // Position::GetOpenDrive()->LoadOpenDriveFile("../../../../large.xodr");
        odrLarge = new OpenDrive("../../../../large.xodr");
    }
    static OpenDrive *odrSmall;
    static OpenDrive *odrMedium;
    static OpenDrive *odrLarge;
};

OpenDrive *FollowRouteTest::odrSmall = nullptr;
OpenDrive *FollowRouteTest::odrMedium = nullptr;
OpenDrive *FollowRouteTest::odrLarge = nullptr;

static void log_callback(const char *str);


TEST_F(FollowRouteTest, FindPathSmallInvalidPosition)
{
    Position::LoadOpenDrive(odrSmall);
    ASSERT_NE(odrSmall, nullptr);

    // Set start pos and the driving direction (heading)
    // PI = against road dir,   0 = road dir
    Position start(0, -1, 10, 0);
    start.SetHeadingRelativeRoadDirection(0);
    Position target(5, -4, 20, 0);

    LaneIndependentRouter router(odrSmall);

    std::vector<Node *> path = router.CalculatePath(start, target);
    // Check invalid target
    ASSERT_TRUE(path.empty());

    start = Position(-1, -1, 10, 0);
    target = Position(5, -2, 20, 0);

    path = router.CalculatePath(start, target);
    // Check invalid start
    ASSERT_TRUE(path.empty());
}

TEST_F(FollowRouteTest, FindPathSmall1)
{
    Position::LoadOpenDrive(odrSmall);
    ASSERT_NE(odrSmall, nullptr);

    // Set start pos and the driving direction (heading)
    // PI = against road dir,   0 = road dir
    Position start(0, -1, 10, 0);
    start.SetHeadingRelativeRoadDirection(0);

    Position target(5, -2, 20, 0);

    LaneIndependentRouter router(odrSmall);
    std::vector<Node *> path = router.CalculatePath(start, target);

    ASSERT_FALSE(path.empty());
    ASSERT_EQ(path.back()->road->GetId(), 5);
}

TEST_F(FollowRouteTest, FindPathSmall2)
{
    Position::LoadOpenDrive(odrSmall);
    ASSERT_NE(odrSmall, nullptr);

    // Set start pos and the driving direction (heading)
    // PI = against road dir,   0 = road dir
    Position start(0, -1, 10, 0);
    start.SetHeadingRelativeRoadDirection(0);

    Position target(2, -1, 20, 0);

    LaneIndependentRouter router(odrSmall);
    std::vector<Node *> path = router.CalculatePath(start, target);

    ASSERT_FALSE(path.empty());
    ASSERT_EQ(path.back()->road->GetId(), 2);
}
TEST_F(FollowRouteTest, FindPathMedium1)
{
    Position::LoadOpenDrive(odrMedium);
    ASSERT_NE(odrMedium, nullptr);

    // Set start pos and the driving direction (heading)
    // PI = against road dir,   0 = road dir
    Position start(202, 2, 100, 0);
    start.SetHeadingRelativeRoadDirection(M_PI);

    Position target(209, 1, 20, 0);

    LaneIndependentRouter router(odrMedium);
    std::vector<Node *> path = router.CalculatePath(start, target);

    ASSERT_FALSE(path.empty());
    ASSERT_EQ(path.back()->road->GetId(), 209);
}

TEST_F(FollowRouteTest, FindPathMedium2)
{
    Position::LoadOpenDrive(odrMedium);
    ASSERT_NE(odrMedium, nullptr);

    // Set start pos and the driving direction (heading)
    // PI = against road dir,   0 = road dir
    Position start(217, -1, 50, 0);
    start.SetHeadingRelativeRoadDirection(0);

    Position target(275, -1, 50, 0);

    LaneIndependentRouter router(odrMedium);
    std::vector<Node *> path = router.CalculatePath(start, target);

    ASSERT_FALSE(path.empty());
    ASSERT_EQ(path.back()->road->GetId(), 275);
}

TEST_F(FollowRouteTest, FindPathLarge1)
{
    Position::LoadOpenDrive(odrLarge);
    ASSERT_NE(odrLarge, nullptr);

    // Set start pos and the driving direction (heading)
    // PI = against road dir,   0 = road dir
    Position start(5147, -1, 50, 0);
    start.SetHeadingRelativeRoadDirection(0);

    Position target(2206, 1, 100, 0);

    LaneIndependentRouter router(odrLarge);
    std::vector<Node *> path = router.CalculatePath(start, target);

    ASSERT_FALSE(path.empty());
    ASSERT_EQ(path.back()->road->GetId(), 2206);
}

TEST_F(FollowRouteTest, FindPathLarge2)
{
    Position::LoadOpenDrive(odrLarge);
    ASSERT_NE(odrLarge, nullptr);

    // Set start pos and the driving direction (heading)
    // PI = against road dir,   0 = road dir
    Position start(1006, 3, 1900, 0);
    start.SetHeadingRelativeRoadDirection(M_PI);

    Position target(2203, 1, 20, 0);

    LaneIndependentRouter router(odrLarge);
    std::vector<Node *> path = router.CalculatePath(start, target);

    ASSERT_FALSE(path.empty());
    ASSERT_EQ(path.back()->road->GetId(), 2203);
}

TEST_F(FollowRouteTest, FindPathLarge3)
{
    Position::LoadOpenDrive(odrLarge);
    ASSERT_NE(odrLarge, nullptr);

    // Set start pos and the driving direction (heading)
    // PI = against road dir,   0 = road dir
    Position start(8277, 1, 200, 0);
    start.SetHeadingRelativeRoadDirection(M_PI);

    Position target(2291, -2, 50, 0);

    LaneIndependentRouter router(odrLarge);
    std::vector<Node *> path = router.CalculatePath(start, target);

    ASSERT_FALSE(path.empty());
    ASSERT_EQ(path.back()->road->GetId(), 2291);
}

TEST_F(FollowRouteTest, FindPathLarge4)
{
    Position::LoadOpenDrive(odrLarge);
    ASSERT_NE(odrLarge, nullptr);

    // Set start pos and the driving direction (heading)
    // PI = against road dir,   0 = road dir
    Position start(2431, 1, 2200, 0);
    start.SetHeadingRelativeRoadDirection(M_PI);

    Position target(2503, -1, 3700, 0);

    LaneIndependentRouter router(odrLarge);
    std::vector<Node *> path = router.CalculatePath(start, target);

    ASSERT_FALSE(path.empty());
    ASSERT_EQ(path.back()->road->GetId(), 2503);
}

TEST_F(FollowRouteTest, FindPathShortest)
{
    Position::LoadOpenDrive(odrMedium);
    ASSERT_NE(odrMedium, nullptr);

    // Set start pos and the driving direction (heading)
    // PI = against road dir,   0 = road dir
    Position start(202, 2, 100, 0);
    start.SetHeadingRelativeRoadDirection(M_PI);

    Position target(209, 1, 20, 0);

    LaneIndependentRouter router(odrMedium);
    std::vector<Node *> path = router.CalculatePath(start, target, RouteStrategy::SHORTEST);

    ASSERT_FALSE(path.empty());
    ASSERT_EQ(path.back()->road->GetId(), 209);
}

TEST_F(FollowRouteTest, FindPathFastest)
{
    Position::LoadOpenDrive(odrMedium);
    ASSERT_NE(odrMedium, nullptr);

    // Set start pos and the driving direction (heading)
    // PI = against road dir,   0 = road dir
    Position start(202, 2, 100, 0);
    start.SetHeadingRelativeRoadDirection(M_PI);

    Position target(209, 1, 20, 0);

    LaneIndependentRouter router(odrMedium);
    std::vector<Node *> path = router.CalculatePath(start, target, RouteStrategy::FASTEST);

    ASSERT_FALSE(path.empty());
    ASSERT_EQ(path.back()->road->GetId(), 209);
}

TEST_F(FollowRouteTest, FindPathMinIntersections)
{
    Position::LoadOpenDrive(odrMedium);
    ASSERT_NE(odrMedium, nullptr);

    // Set start pos and the driving direction (heading)
    // PI = against road dir,   0 = road dir
    Position start(202, 2, 100, 0);
    start.SetHeadingRelativeRoadDirection(M_PI);

    Position target(209, 1, 20, 0);

    LaneIndependentRouter router(odrMedium);
    std::vector<Node *> path = router.CalculatePath(start, target, RouteStrategy::MIN_INTERSECTIONS);
    ASSERT_FALSE(path.empty());
    ASSERT_EQ(path.back()->road->GetId(), 209);
}

TEST_F(FollowRouteTest, FindPathTimeMedium)
{
    Position::LoadOpenDrive(odrMedium);
    ASSERT_NE(odrMedium, nullptr);

    // Set start pos and the driving direction (heading)
    // PI = against road dir,   0 = road dir
    Position start(202, 2, 100, 0);
    start.SetHeadingRelativeRoadDirection(M_PI);

    Position target(209, 1, 20, 0);

    LaneIndependentRouter router(odrMedium);
    auto startTime = std::chrono::high_resolution_clock::now();
    std::vector<Node *> path = router.CalculatePath(start, target, RouteStrategy::SHORTEST);
    auto endTime = std::chrono::high_resolution_clock::now();
    auto elapsedTime = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();

    ASSERT_FALSE(path.empty());
    ASSERT_EQ(path.back()->road->GetId(), 209);

    int maxMicroseconds = 5000;
    ASSERT_LT((int)elapsedTime, maxMicroseconds);
}

TEST_F(FollowRouteTest, FindPathTimeLarge)
{
    Position::LoadOpenDrive(odrLarge);
    ASSERT_NE(odrLarge, nullptr);

    // Set start pos and the driving direction (heading)
    // PI = against road dir,   0 = road dir
    Position start(2291, -1, 1100, 0);
    start.SetHeadingRelativeRoadDirection(0);

    Position target(2502, 1, 50, 0);

    LaneIndependentRouter router(odrLarge);
    auto startTime = std::chrono::high_resolution_clock::now();
    std::vector<Node *> path = router.CalculatePath(start, target, RouteStrategy::SHORTEST);
    auto endTime = std::chrono::high_resolution_clock::now();
    auto elapsedTime = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();

    ASSERT_FALSE(path.empty());
    ASSERT_EQ(path.back()->road->GetId(), 2502);

    int maxMicroseconds = 15000;
    ASSERT_LT((int)elapsedTime, maxMicroseconds);
}

TEST_F(FollowRouteTest, CreateWaypointSmall1)
{
    Position::LoadOpenDrive(odrSmall);
    ASSERT_NE(odrSmall, nullptr);

    std::vector<Position> expectedWaypoints = {
        Position(0, -4, 125, 0),
        Position(4, -1, 50, 0),
        Position(2, -1, 20, 0)};
    // Set start pos and the driving direction (heading)
    // PI = against road dir,   0 = road dir
    Position start(0, -3, 10, 0);
    start.SetHeadingRelativeRoadDirection(0);
    Position target(2, -1, 20, 0);

    LaneIndependentRouter router(odrSmall);
    std::vector<Node *> path = router.CalculatePath(start, target);
    ASSERT_FALSE(path.empty());

    std::vector<Position> calcWaypoints = router.GetWaypoints(path, target);
    ASSERT_FALSE(calcWaypoints.empty());
    ASSERT_EQ(calcWaypoints.size(), expectedWaypoints.size());
    for (int i = 0; i < expectedWaypoints.size(); i++)
    {
        ASSERT_EQ(calcWaypoints[i].GetTrackId(), expectedWaypoints[i].GetTrackId());
        ASSERT_EQ(calcWaypoints[i].GetLaneId(), expectedWaypoints[i].GetLaneId());
        ASSERT_NEAR(calcWaypoints[i].GetS(), expectedWaypoints[i].GetS(), 0.1);
        ASSERT_NEAR(calcWaypoints[i].GetOffset(), expectedWaypoints[i].GetOffset(), 0.1);
    }
}

TEST_F(FollowRouteTest, CreateWaypointSmall2)
{
    Position::LoadOpenDrive(odrSmall);
    ASSERT_NE(odrSmall, nullptr);

    std::vector<Position> expectedWaypoints = {
        Position(0, -3, 100, 0),
        Position(3, -3, 25, 0),
        Position(1, -3, 57.5, 0),
        Position(6, -3, 15, 0),
        Position(5, -3, 20, 0)};

    // Set start pos and the driving direction (heading)
    // PI = against road dir,   0 = road dir
    Position start(0, -3, 10, 0);
    start.SetHeadingRelativeRoadDirection(0);
    Position target(5, -3, 20, 0);

    LaneIndependentRouter router(odrSmall);
    std::vector<Node *> path = router.CalculatePath(start, target);
    ASSERT_FALSE(path.empty());

    std::vector<Position> calcWaypoints = router.GetWaypoints(path, target);
    ASSERT_FALSE(calcWaypoints.empty());
    ASSERT_EQ(calcWaypoints.size(), expectedWaypoints.size());
    for (int i = 0; i < expectedWaypoints.size(); i++)
    {
        ASSERT_EQ(calcWaypoints[i].GetTrackId(), expectedWaypoints[i].GetTrackId());
        ASSERT_EQ(calcWaypoints[i].GetLaneId(), expectedWaypoints[i].GetLaneId());
        ASSERT_NEAR(calcWaypoints[i].GetS(), expectedWaypoints[i].GetS(), 0.1);
        ASSERT_NEAR(calcWaypoints[i].GetOffset(), expectedWaypoints[i].GetOffset(), 0.1);
    }
}

TEST_F(FollowRouteTest, CreateWaypointMedium)
{
    Position::LoadOpenDrive(odrMedium);
    ASSERT_NE(odrMedium, nullptr);

    std::vector<Position> expectedWaypoints = {
        Position(266, 1, 54.5, 0),
        Position(258, -1, 8.85, 0),
        Position(261, -1, 54.5, 0),
        Position(196, 1, 54.5, 0),
        Position(204, -1, 11.5, 0),
        Position(197, -1, 54, 0),
        Position(275, 1, 55, 0)};

    // Set start pos and the driving direction (heading)
    // PI = against road dir,   0 = road dir
    Position start(266, 1, 50, 0);
    start.SetHeadingRelativeRoadDirection(M_PI);
    Position target(275, 1, 55, 0);

    LaneIndependentRouter router(odrMedium);
    std::vector<Node *> path = router.CalculatePath(start, target);
    ASSERT_FALSE(path.empty());

    std::vector<Position> calcWaypoints = router.GetWaypoints(path, target);
    ASSERT_FALSE(calcWaypoints.empty());
    ASSERT_EQ(calcWaypoints.size(), expectedWaypoints.size());
    for (int i = 0; i < expectedWaypoints.size(); i++)
    {
        ASSERT_EQ(calcWaypoints[i].GetTrackId(), expectedWaypoints[i].GetTrackId());
        ASSERT_EQ(calcWaypoints[i].GetLaneId(), expectedWaypoints[i].GetLaneId());
        ASSERT_NEAR(calcWaypoints[i].GetS(), expectedWaypoints[i].GetS(), 0.5);
        ASSERT_NEAR(calcWaypoints[i].GetOffset(), expectedWaypoints[i].GetOffset(), 0.5);
    }
}

TEST_F(FollowRouteTest, CreateWaypointLarge)
{
    Position::LoadOpenDrive(odrLarge);
    ASSERT_NE(odrLarge, nullptr);

    std::vector<Position> expectedWaypoints = {
        Position(5219, -2, 226.8, 0),
        Position(8264, -2, 11, 0),
        Position(2357, -1, 150.6, 0),
        Position(2505, -1, 8.7, 0),
        Position(2704, -1, 21.8, 0),
        Position(2701, -1, 8.7, 0),
        Position(2512, -1, 765.5, 0),
        Position(2203, -1, 17.5, 0)};

    // Set start pos and the driving direction (heading)
    // PI = against road dir,   0 = road dir
    Position start(5219, -1, 116, 0);
    start.SetHeadingRelativeRoadDirection(0);
    Position target(2203, -1, 17.5, 0);

    LaneIndependentRouter router(odrLarge);
    std::vector<Node *> path = router.CalculatePath(start, target);
    ASSERT_FALSE(path.empty());

    std::vector<Position> calcWaypoints = router.GetWaypoints(path, target);
    ASSERT_FALSE(calcWaypoints.empty());
    ASSERT_EQ(calcWaypoints.size(), expectedWaypoints.size());
    for (int i = 0; i < expectedWaypoints.size(); i++)
    {
        ASSERT_EQ(calcWaypoints[i].GetTrackId(), expectedWaypoints[i].GetTrackId());
        ASSERT_EQ(calcWaypoints[i].GetLaneId(), expectedWaypoints[i].GetLaneId());
        ASSERT_NEAR(calcWaypoints[i].GetS(), expectedWaypoints[i].GetS(), 0.5);
        ASSERT_NEAR(calcWaypoints[i].GetOffset(), expectedWaypoints[i].GetOffset(), 0.5);
    }
}


TEST_F(FollowRouteTest, CalcAverageSpeedForRoadsWithoutSpeed)
{
    RoadCalculations roadCalc;

    Road road1(1, "420");
    Road road2(2, "420");
    Road road3(3, "420");
    Road road4(4, "420");
    Road::RoadTypeEntry lowSpeed;
    Road::RoadTypeEntry town;
    Road::RoadTypeEntry rural;
    Road::RoadTypeEntry motorway;
    lowSpeed.road_type_ = Road::RoadType::ROADTYPE_LOWSPEED;
    town.road_type_ = Road::RoadType::ROADTYPE_TOWN;
    rural.road_type_ = Road::RoadType::ROADTYPE_RURAL;
    motorway.road_type_ = Road::RoadType::ROADTYPE_MOTORWAY;

    road1.AddRoadType(&lowSpeed);
    road2.AddRoadType(&town);
    road3.AddRoadType(&rural);
    road4.AddRoadType(&motorway);

    double averageSpeed = roadCalc.CalcAverageSpeed(&road1);
    double expectedSpeed = 8.333;
    ASSERT_NEAR(averageSpeed, expectedSpeed, 0.01);

    averageSpeed = roadCalc.CalcAverageSpeed(&road2);
    expectedSpeed = 13.888;
    ASSERT_NEAR(averageSpeed, expectedSpeed, 0.01);

    averageSpeed = roadCalc.CalcAverageSpeed(&road3);
    expectedSpeed = 19.444;
    ASSERT_NEAR(averageSpeed, expectedSpeed, 0.01);

    averageSpeed = roadCalc.CalcAverageSpeed(&road4);
    expectedSpeed = 25;
    ASSERT_NEAR(averageSpeed, expectedSpeed, 0.01);
}

TEST_F(FollowRouteTest, CalcAverageSpeedForRoadsWithDefinedSpeed)
{
    RoadCalculations roadCalc;

    Road road1(1, "420");
    Road road2(2, "420");
    Road road3(3, "420");
    Road road4(4, "420");
    Road::RoadTypeEntry lowSpeed;
    Road::RoadTypeEntry town;
    Road::RoadTypeEntry rural;
    Road::RoadTypeEntry motorway;
    lowSpeed.road_type_ = Road::RoadType::ROADTYPE_LOWSPEED;
    lowSpeed.speed_ = 10;
    town.road_type_ = Road::RoadType::ROADTYPE_TOWN;
    town.speed_ = 20;
    rural.road_type_ = Road::RoadType::ROADTYPE_RURAL;
    rural.speed_ = 30;
    motorway.road_type_ = Road::RoadType::ROADTYPE_MOTORWAY;
    motorway.speed_ = 40;

    road1.AddRoadType(&lowSpeed);
    road2.AddRoadType(&town);
    road3.AddRoadType(&rural);
    road4.AddRoadType(&motorway);

    double averageSpeed = roadCalc.CalcAverageSpeed(&road1);
    double expectedSpeed = 10;
    ASSERT_NEAR(averageSpeed, expectedSpeed, 0.01);

    averageSpeed = roadCalc.CalcAverageSpeed(&road2);
    expectedSpeed = 20;
    ASSERT_NEAR(averageSpeed, expectedSpeed, 0.01);

    averageSpeed = roadCalc.CalcAverageSpeed(&road3);
    expectedSpeed = 30;
    ASSERT_NEAR(averageSpeed, expectedSpeed, 0.01);

    averageSpeed = roadCalc.CalcAverageSpeed(&road4);
    expectedSpeed = 40;
    ASSERT_NEAR(averageSpeed, expectedSpeed, 0.01);
}

TEST_F(FollowRouteTest, CalcAverageSpeedForTwoRoadTypes)
{
    RoadCalculations roadCalc;

    Road road(1, "420");
    Road::RoadTypeEntry lowSpeed;
    Road::RoadTypeEntry town;
    lowSpeed.road_type_ = Road::RoadType::ROADTYPE_LOWSPEED;
    town.road_type_ = Road::RoadType::ROADTYPE_TOWN;

    road.AddRoadType(&lowSpeed);
    road.AddRoadType(&town);

    double averageSpeed = roadCalc.CalcAverageSpeed(&road);
    double expectedSpeed = 11.11;
    ASSERT_NEAR(averageSpeed, expectedSpeed, 0.01);

    lowSpeed.speed_ = 10;
    town.speed_ = 20;

    averageSpeed = roadCalc.CalcAverageSpeed(&road);
    expectedSpeed = 15;
    ASSERT_NEAR(averageSpeed, expectedSpeed, 0.01);
}


// Uncomment to print log output to console
#define LOG_TO_CONSOLE

#ifdef LOG_TO_CONSOLE
static void log_callback(const char *str)
{
    printf("%s\n", str);
}
#endif

int main(int argc, char **argv)
{
#ifdef LOG_TO_CONSOLE
    if (!(Logger::Inst().IsCallbackSet()))
    {
        Logger::Inst().SetCallback(log_callback);
    }
#endif

    // testing::GTEST_FLAG(filter) = "*TestOptionHandling*";

    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}