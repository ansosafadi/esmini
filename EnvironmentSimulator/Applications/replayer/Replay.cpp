/*
 * esmini - Environment Simulator Minimalistic
 * https://github.com/esmini/esmini
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) partners of Simulation Scenarios
 * https://sites.google.com/view/simulationscenarios
 */
#include <filesystem>

#include "Replay.hpp"
#include "ScenarioGateway.hpp"
#include "CommonMini.hpp"
#include "dirent.h"

using namespace scenarioengine;

// new replayer constructor
Replay::Replay(std::string filename) : time_(0.0), index_(0), repeat_(false)
{
    std::filesystem::path cwd = std::filesystem::current_path();
    std::cout << "REPLAY: " << cwd << std::endl;
    file_.open(filename, std::ofstream::binary);
    if (file_.fail())
    {
        LOG("Cannot open file: %s", filename.c_str());
        throw std::invalid_argument(std::string("Cannot open file: ") + filename);
    }

    RecordPkgs(filename);

    headerNew_ = *reinterpret_cast<datLogger::DatHdr*>(pkgs_[0].content);

    LOG("Recording %s opened. dat version: %d odr: %s model: %s",
        FileNameOf(filename).c_str(),
        headerNew_.version,
        FileNameOf(headerNew_.odrFilename.string).c_str(),
        FileNameOf(headerNew_.modelFilename.string).c_str());

    if (headerNew_.version != DAT_FILE_FORMAT_VERSION)
    {
        LOG_AND_QUIT("Version mismatch. %s is version %d while supported version is %d. Please re-create dat file.",
                     filename.c_str(),
                     headerNew_.version,
                     DAT_FILE_FORMAT_VERSION);
    }

    if (pkgs_.size() > 0)
    {
        // Register first entry timestamp as starting time
        time_       = *reinterpret_cast<double*>(pkgs_[1].content);
        startTime_  = time_;
        startIndex_ = 1;

        // Register last entry timestamp as stop time
        SetStopEntries();
    }
    // initiate the cache with first time frame
    InitiateStates();
}

Replay::Replay(std::string filename, bool clean) : time_(0.0), index_(0), repeat_(false), clean_(clean)
{
    file_.open(filename, std::ofstream::binary);
    if (file_.fail())
    {
        LOG("Cannot open file: %s", filename.c_str());
        throw std::invalid_argument(std::string("Cannot open file: ") + filename);
    }

    file_.read(reinterpret_cast<char*>(&header_), sizeof(header_));
    LOG("Recording %s opened. dat version: %d odr: %s model: %s",
        FileNameOf(filename).c_str(),
        header_.version,
        FileNameOf(header_.odr_filename).c_str(),
        FileNameOf(header_.model_filename).c_str());

    if (header_.version != DAT_FILE_FORMAT_VERSION)
    {
        LOG_AND_QUIT("Version mismatch. %s is version %d while supported version is %d. Please re-create dat file.",
                     filename.c_str(),
                     header_.version,
                     DAT_FILE_FORMAT_VERSION);
    }

    if (header_.version != DAT_FILE_FORMAT_VERSION)
    {
        LOG_AND_QUIT("Version mismatch. %s is version %d while supported version is %d. Please re-create dat file.",
                     filename.c_str(),
                     header_.version,
                     DAT_FILE_FORMAT_VERSION);
    }

    if (header_.version != 2)
    {
        LOG_AND_QUIT("Version mismatch. %s is version %d while supported version is %d. Please re-create dat file.",
                     filename.c_str(),
                     header_.version,
                     DAT_FILE_FORMAT_VERSION);
    }

    while (!file_.eof())
    {
        ReplayEntry data;

        file_.read(reinterpret_cast<char*>(&data.state), sizeof(data.state));

        if (!file_.eof())
        {
            data_.push_back(data);
        }
    }

    if (clean_)
    {
        CleanEntries(data_);
    }

    if (data_.size() > 0)
    {
        // Register first entry timestamp as starting time
        time_       = data_[0].state.info.timeStamp;
        startTime_  = time_;
        startIndex_ = 0;

        // Register last entry timestamp as stop time
        stopTime_  = data_.back().state.info.timeStamp;
        stopIndex_ = static_cast<unsigned int>(FindIndexAtTimestamp(stopTime_));
    }
}

Replay::Replay(const std::string directory, const std::string scenario, std::string create_datfile)
    : time_(0.0),
      index_(0),
      repeat_(false),
      create_datfile_(create_datfile)
{
    GetReplaysFromDirectory(directory, scenario);
    std::vector<std::pair<std::string, std::vector<ReplayEntry>>> scenarioData;

    for (size_t i = 0; i < scenarios_.size(); i++)
    {
        file_.open(scenarios_[i], std::ofstream::binary);
        if (file_.fail())
        {
            LOG("Cannot open file: %s", scenarios_[i].c_str());
            throw std::invalid_argument(std::string("Cannot open file: ") + scenarios_[i]);
        }
        file_.read(reinterpret_cast<char*>(&header_), sizeof(header_));
        LOG("Recording %s opened. dat version: %d odr: %s model: %s",
            FileNameOf(scenarios_[i]).c_str(),
            header_.version,
            FileNameOf(header_.odr_filename).c_str(),
            FileNameOf(header_.model_filename).c_str());

        if (header_.version != DAT_FILE_FORMAT_VERSION)
        {
            LOG_AND_QUIT("Version mismatch. %s is version %d while supported version is %d. Please re-create dat file.",
                         scenarios_[i].c_str(),
                         header_.version,
                         DAT_FILE_FORMAT_VERSION);
        }
        while (!file_.eof())
        {
            ReplayEntry entry;

            file_.read(reinterpret_cast<char*>(&entry.state), sizeof(entry.state));

            if (!file_.eof())
            {
                data_.push_back(entry);
            }
        }
        // pair <scenario name, scenario data>
        scenarioData.push_back(std::make_pair(scenarios_[i], data_));
        data_ = {};
        file_.close();
    }

    if (scenarioData.size() < 2)
    {
        LOG_AND_QUIT("Too few scenarios loaded, use single replay feature instead\n");
    }

    // Scenario with smallest start time first
    std::sort(scenarioData.begin(),
              scenarioData.end(),
              [](const auto& sce1, const auto& sce2) { return sce1.second[0].state.info.timeStamp < sce2.second[0].state.info.timeStamp; });

    // Log which scenario belongs to what ID-group (0, 100, 200 etc.)
    for (size_t i = 0; i < scenarioData.size(); i++)
    {
        std::string scenario_tmp = scenarioData[i].first;
        LOG("Scenarios corresponding to IDs (%d:%d): %s", i * 100, (i + 1) * 100 - 1, FileNameOf(scenario_tmp.c_str()).c_str());
    }

    // Ensure increasing timestamps. Remove any other entries.
    for (auto& sce : scenarioData)
    {
        CleanEntries(sce.second);
    }

    // Build remaining data in order.
    BuildData(scenarioData);

    if (data_.size() > 0)
    {
        // Register first entry timestamp as starting time
        time_       = data_[0].state.info.timeStamp;
        startTime_  = time_;
        startIndex_ = 0;

        // Register last entry timestamp as stop time
        stopTime_  = data_.back().state.info.timeStamp;
        stopIndex_ = static_cast<unsigned int>(FindIndexAtTimestamp(stopTime_));
    }

    if (!create_datfile_.empty())
    {
        CreateMergedDatfile(create_datfile_);
    }
}

// Browse through replay-folder and appends strings of absolute path to matching scenario
void Replay::GetReplaysFromDirectory(const std::string dir, const std::string sce)
{
    DIR* directory = opendir(dir.c_str());

    // If no directory found, write error
    if (directory == nullptr)
    {
        LOG_AND_QUIT("No valid directory given, couldn't open %s", dir.c_str());
    }

    // While directory is open, check the filename
    struct dirent* file;
    while ((file = readdir(directory)) != nullptr)
    {
        std::string filename = file->d_name;
        if (file->d_type == DT_DIR && filename.find(sce) != std::string::npos)
        {
            DIR* nested_dir = opendir((dir + filename).c_str());
            if (nested_dir == nullptr)
            {
                LOG("Couldn't open nested directory %s", (dir + filename).c_str());
            }

            struct dirent* nested_file;
            while ((nested_file = readdir(nested_dir)) != nullptr)
            {
                std::string nested_filename = nested_file->d_name;

                if (nested_filename != "." && nested_filename != ".." && nested_filename.find(sce) != std::string::npos &&
                    nested_filename.find(".dat") != std::string::npos)
                {
                    scenarios_.emplace_back(CombineDirectoryPathAndFilepath(dir + filename, nested_filename));
                }
            }
            closedir(nested_dir);
        }

        if (filename != "." && filename != ".." && filename.find(sce) != std::string::npos && filename.find(".dat") != std::string::npos)
        {
            scenarios_.emplace_back(CombineDirectoryPathAndFilepath(dir, filename));
        }
    }
    closedir(directory);

    // Sort list of filenames
    std::sort(scenarios_.begin(), scenarios_.end(), [](std::string const& a, std::string const& b) { return a < b; });

    if (scenarios_.empty())
    {
        LOG_AND_QUIT("Couldn't read any scenarios named %s in path %s", sce.c_str(), dir.c_str());
    }
}

size_t Replay::GetNumberOfScenarios()
{
    return scenarios_.size();
}

Replay::~Replay()
{
    data_.clear();
}

void Replay::MoveToStart()
{
    MoveToTime(startTime_);
}

void Replay::MoveToEnd()
{
    MoveToTime(stopTime_);
}

int Replay::FindIndexAtTimestamp(double timestamp, int startSearchIndex)
{
    int i = 0;

    if (timestamp > stopTime_)
    {
        MoveToEnd();
        return static_cast<int>(index_);
    }
    else if (timestamp < GetStartTime())
    {
        return static_cast<int>(index_);
    }

    if (timestamp < time_)
    {
        // start search from beginning
        startSearchIndex = 0;
    }

    for (i = startSearchIndex; i < static_cast<int>(pkgs_.size()); i++)
    {
        if (static_cast<datLogger::PackageId>(pkgs_[static_cast<unsigned int>(i)].hdr.id) == datLogger::PackageId::TIME_SERIES)
        {
            double timeTemp = *reinterpret_cast<double*>(pkgs_[static_cast<unsigned int>(i)].content);
            if ( isEqualDouble(timeTemp, timestamp) || (timeTemp > timestamp))
            {
                break;
            }
        }

    }

    return MIN(i, static_cast<int>(pkgs_.size()) - 1);
}

// new replayer

int Replay::RecordPkgs(const std::string& fileName)
{
    std::ifstream file_Read_;
    file_Read_.open(fileName, std::ifstream::binary);
    if (file_Read_.fail())
    {
        std::printf("READ, Cannot open file: %s", fileName.c_str());
        return -1;
    }

    if (file_Read_.is_open())
    {
        std::cout << "File Opened for read" << std::endl;
    }

    // Get the file size
    file_Read_.seekg(0, std::ios::end);
    std::streampos file_size = file_Read_.tellg();
    file_Read_.seekg(0, std::ios::beg);
    while (file_Read_.good())
    {
        if (file_Read_.tellg() == file_size)
        {
            break;
        }
        // read the header for every loop
        datLogger::CommonPkgHdr cmnHdrPkgRead;
        file_Read_.read(reinterpret_cast<char*>(&cmnHdrPkgRead), sizeof(datLogger::CommonPkgHdr));
        switch (static_cast<datLogger::PackageId>(cmnHdrPkgRead.id))
        {
            case datLogger::PackageId::HEADER:
            {
                datLogger::CommonPkg hdrPkgRead;
                hdrPkgRead.hdr = cmnHdrPkgRead;

                datLogger::DatHdr* datHdrRead = new datLogger::DatHdr;
                // Read content -> version
                file_Read_.read(reinterpret_cast<char*>(&datHdrRead->version), sizeof(datLogger::DatHdr::version));

                datLogger::CommonString odrStrRead;
                // Read content -> odr filename size
                file_Read_.read(reinterpret_cast<char*>(&odrStrRead.size), sizeof(odrStrRead.size));

                // Read content -> odr filename string
                odrStrRead.string = new char[odrStrRead.size];
                file_Read_.read(odrStrRead.string, odrStrRead.size);

                datLogger::CommonString mdlStrRead;
                // Read content -> model filename size
                file_Read_.read(reinterpret_cast<char*>(&mdlStrRead.size), sizeof(mdlStrRead.size));

                // Read content -> model filename string
                mdlStrRead.string = new char[mdlStrRead.size];
                file_Read_.read(mdlStrRead.string, mdlStrRead.size);

                datHdrRead->odrFilename   = odrStrRead;
                datHdrRead->modelFilename = mdlStrRead;
                hdrPkgRead.content        = reinterpret_cast<char*>(datHdrRead);

                pkgs_.push_back(hdrPkgRead);

                break;
            }

            case datLogger::PackageId::MODEL_ID:
            {
                datLogger::CommonPkg modelIdPkgRead;
                modelIdPkgRead.hdr = cmnHdrPkgRead;

                datLogger::ModelId* modelId = new datLogger::ModelId;
                file_Read_.read(reinterpret_cast<char*>(&modelId->model_id), modelIdPkgRead.hdr.content_size);
                modelIdPkgRead.content = reinterpret_cast<char*>(modelId);
                pkgs_.push_back(modelIdPkgRead);
                break;
            }

            case datLogger::PackageId::TIME_SERIES:
            {
                datLogger::CommonPkg timePkgRead;
                timePkgRead.hdr = cmnHdrPkgRead;

                datLogger::Time* t = new datLogger::Time;
                file_Read_.read(reinterpret_cast<char*>(&t->time), timePkgRead.hdr.content_size);
                timePkgRead.content = reinterpret_cast<char*>(t);
                pkgs_.push_back(timePkgRead);
                if (!isEqualDouble(t->time, 0.0))
                {
                    if (fabs(t->time - perviousTime_) < deltaTime_)
                    {
                        deltaTime_ = fabs(t->time - perviousTime_);
                    }
                }
                printf("current time %.2f pervious time %.2f delta time %.2f\n", t->time, perviousTime_, deltaTime_);
                perviousTime_ = t->time;
                break;
            }

            case datLogger::PackageId::OBJ_ID:
            {
                datLogger::CommonPkg objIdPkgRead;
                objIdPkgRead.hdr = cmnHdrPkgRead;

                datLogger::ObjId* objIdRead = new datLogger::ObjId;
                file_Read_.read(reinterpret_cast<char*>(&objIdRead->obj_id), objIdPkgRead.hdr.content_size);
                objIdPkgRead.content = reinterpret_cast<char*>(objIdRead);
                pkgs_.push_back(objIdPkgRead);
                break;
            }

            case datLogger::PackageId::POSITIONS:
            {
                datLogger::CommonPkg posPkgRead;
                posPkgRead.hdr = cmnHdrPkgRead;

                datLogger::Pos* posRead = new datLogger::Pos;
                // file_Read_.read(reinterpret_cast<char*>(&posRead), posPkgRead.hdr.content_size);
                file_Read_.read(reinterpret_cast<char*>(&posRead->x), sizeof(datLogger::Pos::x));
                file_Read_.read(reinterpret_cast<char*>(&posRead->y), sizeof(datLogger::Pos::y));
                file_Read_.read(reinterpret_cast<char*>(&posRead->z), sizeof(datLogger::Pos::z));
                file_Read_.read(reinterpret_cast<char*>(&posRead->h), sizeof(datLogger::Pos::h));
                file_Read_.read(reinterpret_cast<char*>(&posRead->r), sizeof(datLogger::Pos::r));
                file_Read_.read(reinterpret_cast<char*>(&posRead->p), sizeof(datLogger::Pos::p));
                posPkgRead.content = reinterpret_cast<char*>(posRead);
                pkgs_.push_back(posPkgRead);
                break;
            }

            case datLogger::PackageId::SPEED:
            {
                datLogger::CommonPkg speedPkgRead;
                speedPkgRead.hdr = cmnHdrPkgRead;

                datLogger::Speed* SpeedRead = new datLogger::Speed;
                file_Read_.read(reinterpret_cast<char*>(&SpeedRead->speed_), speedPkgRead.hdr.content_size);
                speedPkgRead.content = reinterpret_cast<char*>(SpeedRead);
                pkgs_.push_back(speedPkgRead);
                break;
            }

            case datLogger::PackageId::OBJ_TYPE:
            {
                datLogger::CommonPkg objTypePkgRead;
                objTypePkgRead.hdr = cmnHdrPkgRead;

                datLogger::ObjType* objType = new datLogger::ObjType;
                file_Read_.read(reinterpret_cast<char*>(&objType->obj_type), objTypePkgRead.hdr.content_size);
                objTypePkgRead.content = reinterpret_cast<char*>(objType);
                pkgs_.push_back(objTypePkgRead);
                break;
            }

            case datLogger::PackageId::OBJ_CATEGORY:
            {
                datLogger::CommonPkg objCatPkgRead;
                objCatPkgRead.hdr = cmnHdrPkgRead;

                datLogger::ObjCategory * objCategory = new datLogger::ObjCategory;
                file_Read_.read(reinterpret_cast<char*>(&objCategory->obj_category), objCatPkgRead.hdr.content_size);
                objCatPkgRead.content = reinterpret_cast<char*>(objCategory);
                pkgs_.push_back(objCatPkgRead);
                break;
            }

            case datLogger::PackageId::CTRL_TYPE:
            {
                datLogger::CommonPkg ctrlTypePkgRead;
                ctrlTypePkgRead.hdr = cmnHdrPkgRead;

                datLogger::CtrlType* ctrlType = new datLogger::CtrlType;
                file_Read_.read(reinterpret_cast<char*>(&ctrlType->ctrl_type), ctrlTypePkgRead.hdr.content_size);
                ctrlTypePkgRead.content = reinterpret_cast<char*>(ctrlType);
                pkgs_.push_back(ctrlTypePkgRead);
                break;
            }

            case datLogger::PackageId::WHEEL_ANGLE:
            {
                datLogger::CommonPkg wheelAnglePkgRead;
                wheelAnglePkgRead.hdr = cmnHdrPkgRead;

                datLogger::WheelAngle * wheelAngle = new datLogger::WheelAngle;
                file_Read_.read(reinterpret_cast<char*>(&wheelAngle->wheel_angle), wheelAnglePkgRead.hdr.content_size);
                wheelAnglePkgRead.content = reinterpret_cast<char*>(wheelAngle);
                pkgs_.push_back(wheelAnglePkgRead);
                break;
            }

            case datLogger::PackageId::WHEEL_ROT:
            {
                datLogger::CommonPkg wheelRotPkgRead;
                wheelRotPkgRead.hdr = cmnHdrPkgRead;

                datLogger::WheelRot * wheelRot = new datLogger::WheelRot;
                file_Read_.read(reinterpret_cast<char*>(&wheelRot->wheel_rot), wheelRotPkgRead.hdr.content_size);
                wheelRotPkgRead.content = reinterpret_cast<char*>(wheelRot);
                pkgs_.push_back(wheelRotPkgRead);
                break;
            }

            case datLogger::PackageId::BOUNDING_BOX:
            {
                datLogger::CommonPkg objTypePkgRead;
                objTypePkgRead.hdr = cmnHdrPkgRead;

                datLogger::BoundingBox* bb = new datLogger::BoundingBox;
                file_Read_.read(reinterpret_cast<char*>(&bb->x), sizeof(datLogger::BoundingBox::x));
                file_Read_.read(reinterpret_cast<char*>(&bb->y), sizeof(datLogger::BoundingBox::y));
                file_Read_.read(reinterpret_cast<char*>(&bb->z), sizeof(datLogger::BoundingBox::z));
                file_Read_.read(reinterpret_cast<char*>(&bb->width), sizeof(datLogger::BoundingBox::width));
                file_Read_.read(reinterpret_cast<char*>(&bb->length), sizeof(datLogger::BoundingBox::length));
                file_Read_.read(reinterpret_cast<char*>(&bb->height), sizeof(datLogger::BoundingBox::height));
                objTypePkgRead.content = reinterpret_cast<char*>(bb);
                pkgs_.push_back(objTypePkgRead);
                break;
            }

            case datLogger::PackageId::SCALE_MODE:
            {
                datLogger::CommonPkg scaleModePkgRead;
                scaleModePkgRead.hdr = cmnHdrPkgRead;

                datLogger::ScaleMode * scaleMode = new datLogger::ScaleMode;
                file_Read_.read(reinterpret_cast<char*>(&scaleMode->scale_mode), scaleModePkgRead.hdr.content_size);
                scaleModePkgRead.content = reinterpret_cast<char*>(scaleMode);
                pkgs_.push_back(scaleModePkgRead);
                break;
            }

            case datLogger::PackageId::VISIBILITY_MASK:
            {
                datLogger::CommonPkg maskPkgRead;
                maskPkgRead.hdr = cmnHdrPkgRead;

                datLogger::VisibilityMask * mask = new datLogger::VisibilityMask;
                file_Read_.read(reinterpret_cast<char*>(&mask->visibility_mask), maskPkgRead.hdr.content_size);
                maskPkgRead.content = reinterpret_cast<char*>(mask);
                pkgs_.push_back(maskPkgRead);
                break;
            }

            case datLogger::PackageId::NAME:
            {
                datLogger::CommonPkg namePkgRead;
                namePkgRead.hdr = cmnHdrPkgRead;

                datLogger::Name* nameStrRead = new  datLogger::Name;

                nameStrRead->string = new char[namePkgRead.hdr.content_size];
                file_Read_.read(nameStrRead->string, namePkgRead.hdr.content_size);
                namePkgRead.content = reinterpret_cast<char*>(nameStrRead);
                pkgs_.push_back(namePkgRead);
                break;
            }

            case datLogger::PackageId::ROAD_ID:
            {
                datLogger::CommonPkg pkg;
                pkg.hdr = cmnHdrPkgRead;

                datLogger::RoadId * road_id = new datLogger::RoadId;
                file_Read_.read(reinterpret_cast<char*>(&road_id->road_id), pkg.hdr.content_size);
                pkg.content = reinterpret_cast<char*>(road_id);
                pkgs_.push_back(pkg);
                break;
            }

            case datLogger::PackageId::LANE_ID:
            {
                datLogger::CommonPkg pkg;
                pkg.hdr = cmnHdrPkgRead;

                datLogger::LaneId * lane_id = new datLogger::LaneId;
                file_Read_.read(reinterpret_cast<char*>(&lane_id->lane_id), pkg.hdr.content_size);
                pkg.content = reinterpret_cast<char*>(lane_id);
                pkgs_.push_back(pkg);
                break;
            }

            case datLogger::PackageId::POS_OFFSET:
            {
                datLogger::CommonPkg pkg;
                pkg.hdr = cmnHdrPkgRead;

                datLogger::PosOffset * pos_offset = new datLogger::PosOffset;
                file_Read_.read(reinterpret_cast<char*>(&pos_offset->offset), pkg.hdr.content_size);
                pkg.content = reinterpret_cast<char*>(pos_offset);
                pkgs_.push_back(pkg);
                break;
            }

            case datLogger::PackageId::POS_T:
            {
                datLogger::CommonPkg pkg;
                pkg.hdr = cmnHdrPkgRead;

                datLogger::PosT * pos_t = new datLogger::PosT;
                file_Read_.read(reinterpret_cast<char*>(&pos_t->t), pkg.hdr.content_size);
                pkg.content = reinterpret_cast<char*>(pos_t);
                pkgs_.push_back(pkg);
                break;
            }

            case datLogger::PackageId::POS_S:
            {
                datLogger::CommonPkg pkg;
                pkg.hdr = cmnHdrPkgRead;

                datLogger::PosS * pos_s = new datLogger::PosS;
                file_Read_.read(reinterpret_cast<char*>(&pos_s->s), pkg.hdr.content_size);
                pkg.content = reinterpret_cast<char*>(pos_s);
                pkgs_.push_back(pkg);
                break;
            }
            case datLogger::PackageId::OBJ_DELETED:
            {
                datLogger::CommonPkg pkg;
                pkg.hdr = cmnHdrPkgRead;

                pkg.content = nullptr;
                pkgs_.push_back(pkg);
                break;
            }

            case datLogger::PackageId::OBJ_ADDED:
            {
                datLogger::CommonPkg pkg;
                pkg.hdr = cmnHdrPkgRead;

                pkg.content = nullptr;
                pkgs_.push_back(pkg);
                break;
            }

            case datLogger::PackageId::END_OF_SCENARIO:
            {
                datLogger::CommonPkg pkg;
                pkg.hdr = cmnHdrPkgRead;

                pkg.content = nullptr;
                pkgs_.push_back(pkg);
                break;
            }

            default:
            {
                std::cout << "Unknown package read->package id :" << std::endl;
                break;
            }
        }
    }
    return 0;
}

datLogger::PackageId Replay::ReadPkgHdr(char* package)
{
    datLogger::CommonPkg pkg;
    pkg = *reinterpret_cast<datLogger::CommonPkg*>(package);
    // std::cout << "Found package ID from current state: " << pkgIdTostring( static_cast<PackageId>(pkg.hdr.id)) << std::endl;
    return static_cast<datLogger::PackageId>(pkg.hdr.id);
}

int Replay::GetPkgCntBtwObj(size_t idx)
{
    int count = 0;
    for (size_t i = idx + 1; i < pkgs_.size(); i++)  // start looking from next package
    {
        if (static_cast<datLogger::PackageId>(pkgs_[i].hdr.id) == datLogger::PackageId::TIME_SERIES ||
            static_cast<datLogger::PackageId>(pkgs_[i].hdr.id) == datLogger::PackageId::OBJ_ID)
        {
            break;  // stop looking if time or obj id package found
        }
        count += 1;  // count package
    }
    return count;
}

std::vector<int> Replay::GetNumberOfObjectsAtTime()
{
    std::vector<int> Indices;
    bool             timeFound = false;

    for (size_t i = index_; i < pkgs_.size(); i++)
    {
        if (pkgs_[i].hdr.id == static_cast<int>(datLogger::PackageId::TIME_SERIES) && !timeFound)
        {
            double timeTemp = *reinterpret_cast<double*>(pkgs_[i].content);
            if  (isEqualDouble(timeTemp, time_))
            {
                timeFound = true;
            }
            continue;  // continue till time match found. if time matched then
        }
        if (timeFound && static_cast<datLogger::PackageId>(pkgs_[i].hdr.id) == datLogger::PackageId::OBJ_ID)
        {
            Indices.push_back(static_cast<int>(i));  // time matches
        }
        if (pkgs_[i].hdr.id == static_cast<int>(datLogger::PackageId::TIME_SERIES))
        {
            return Indices;  // second time instances
        }
    }
    return Indices;
}


bool Replay::IsObjAvailableInCache(int id)  // check in current state
{
    bool status = false;
    for (size_t i = 0; i < scenarioState.obj_states.size(); i++)  // loop current state object id to find the object id
    {
        if (scenarioState.obj_states[i].id == id)
        {
            status = true;  // obj id present
            break;
        }
    }
    return status;
}

bool Replay::IsObjAvailableActive(int id)  // check in current state
{
    bool status = false;
    for (size_t i = 0; i < scenarioState.obj_states.size(); i++)  // loop current state object id to find the object id
    {
        if (scenarioState.obj_states[i].id == id)
        {
            if (scenarioState.obj_states[i].active == true)
            {
                status = true;  // obj id present
                break;
            }
        }
    }
    return status;
}

int  Replay::UpdateObjStatus( int id, bool status)
{
    for (size_t i = 0; i < scenarioState.obj_states.size(); i++)  // loop current state object id to find the object id
    {
        if (scenarioState.obj_states[i].id == id)
        {
            scenarioState.obj_states[i].active = status;
        }
    }
    return 0;
}
void Replay::MoveToDeltaTime(double dt)
{
    MoveToTime(scenarioState.sim_time + dt);
}


void Replay::MoveToNextFrame()
{
    for (size_t i = static_cast<size_t>(index_); i < pkgs_.size(); i++)
    {
        if (pkgs_[i].hdr.id == static_cast<int>(datLogger::PackageId::TIME_SERIES))
        {
            double timeTemp = *reinterpret_cast<double*>(pkgs_[i].content);
            if  ( timeTemp > time_)
            {
                index_ = static_cast<unsigned int>(i);
                time_ = timeTemp;
                break;
            }
        }
    }
}

void Replay::MoveToPreviousFrame()
{
    for (size_t i = static_cast<size_t>(index_); static_cast<int>(i) > 0; i--)
    {
        if (pkgs_[i].hdr.id == static_cast<int>(datLogger::PackageId::TIME_SERIES))
        {
            double timeTemp = *reinterpret_cast<double*>(pkgs_[i].content);
            if  ( timeTemp < time_)
            {
                index_ = static_cast<unsigned int>(i);
                time_ = timeTemp;
                break;
            }
        }
    }
}

void Replay::UpdateCache()
{
    std::vector<int> objIdIndices = GetNumberOfObjectsAtTime();
    for (size_t l = 0; l < objIdIndices.size(); l++)
    {
        int obj_id = *reinterpret_cast<int*>(pkgs_[static_cast<size_t>(objIdIndices[l])].content);
        int pkgCnt = GetPkgCntBtwObj(static_cast<size_t>(objIdIndices[l]));
        for (size_t i = 0; i < scenarioState.obj_states.size(); i++)  // loop current state object id to find the object id
        {
            if (scenarioState.obj_states[i].id == obj_id)  // found object id
            {
                for (size_t j = 0; j < scenarioState.obj_states[i].pkgs.size();j++)
                {
                    for (size_t k = static_cast<size_t>(objIdIndices[l] + 1);
                            k < static_cast<size_t>(objIdIndices[l] + pkgCnt + 1);
                            k++)  // start with Index + 1, Index will have object id package index. looking from next package
                    {
                        datLogger::PackageId id_ = ReadPkgHdr(scenarioState.obj_states[i].pkgs[j].pkg);
                        if (id_ == static_cast<datLogger::PackageId>(pkgs_[k].hdr.id))
                        {
                            scenarioState.obj_states[i].pkgs[j].pkg   = reinterpret_cast<char*>(&pkgs_[k]);
                            scenarioState.obj_states[i].pkgs[j].time_ = time_;
                            break;
                        }
                    }
                }
            }
        }
    }
}

int Replay::MoveToTime(double t)
{
    if ( isEqualDouble(t, scenarioState.sim_time))
    {
        return 0; // no update to cache
    }
    else
    {
        if (scenarioState.sim_time < t)
        {
            while ( t > time_ )
            {
                if( isEqualDouble(stopTime_, time_))
                {
                    break;
                }
                MoveToNextFrame();
                if( t < time_ )
                {
                    MoveToPreviousFrame();
                    break;
                }
                std::vector<int> objIdIndices = GetNumberOfObjectsAtTime();
                for (size_t Index = 0; Index < objIdIndices.size(); Index++)
                {
                    int obj_id = *reinterpret_cast<int*>(pkgs_[static_cast<size_t>(objIdIndices[Index])].content);
                    if (pkgs_[static_cast<size_t>(objIdIndices[Index] + 1)].hdr.id == static_cast<int>(datLogger::PackageId::OBJ_DELETED))
                    {
                        UpdateObjStatus( obj_id, false); // obj deleted in cache
                        continue;
                    }
                    else if (pkgs_[static_cast<size_t>(objIdIndices[Index] + 1)].hdr.id == static_cast<int>(datLogger::PackageId::OBJ_ADDED))
                    {
                        if (!IsObjAvailableInCache(obj_id))
                        {
                            AddObjState(static_cast<size_t>(objIdIndices[Index]), t); // obj added in cache. this will also take updating pkgs
                            continue;
                        }
                        UpdateObjStatus( obj_id, true);
                    }
                }
                UpdateCache();
                if (isEqualDouble(t, time_))
                {
                    break;
                }
            }
        }
        if ( scenarioState.sim_time > t)
        {
            while ( t < time_ )
            {
                if( isEqualDouble(startTime_, time_))
                {
                    break;
                }
                std::vector<int> objIdIndices = GetNumberOfObjectsAtTime();
                // check all obj in this time frame also in cache
                for (size_t Index = 0; Index < objIdIndices.size(); Index++)
                {
                    int obj_id = *reinterpret_cast<int*>(pkgs_[static_cast<size_t>(objIdIndices[Index])].content);
                    if (pkgs_[static_cast<size_t>(objIdIndices[Index] + 1)].hdr.id == static_cast<int>(datLogger::PackageId::OBJ_DELETED))
                    {
                        if (!IsObjAvailableInCache(obj_id))
                        {
                            AddObjState(static_cast<size_t>(objIdIndices[Index]), t); // obj added in cache. this will also take updating pkgs
                            continue;
                        }
                        UpdateObjStatus( obj_id, true); // obj deleted in cache
                    }
                    else if (pkgs_[static_cast<size_t>(objIdIndices[Index] + 1)].hdr.id == static_cast<int>(datLogger::PackageId::OBJ_ADDED))
                    {
                        UpdateObjStatus( obj_id, false);
                    }
                }
                UpdateCache();
                MoveToPreviousFrame();
                if(( t > time_ ) || (isEqualDouble(t, time_)))
                {
                    break;
                }
            }
        }
    }
    if (t > stopTime_)
    {
       scenarioState.sim_time = stopTime_;
       index_ = stopIndex_;
    }
    else if ( t < startTime_)
    {
        scenarioState.sim_time = startTime_;
        index_ = startIndex_;
    }
    else
    {
        scenarioState.sim_time = t;
    }
    printf("Time %.2f Target time %.2f scenario time %.2f\n", time_, t, scenarioState.sim_time);
    return 0;
}

void Replay::SetStopEntries()
{
    for (size_t i = pkgs_.size() - 1; i > 0; i--)
    {
        if (static_cast<datLogger::PackageId>(pkgs_[i].hdr.id) == datLogger::PackageId::TIME_SERIES)
        {
            stopTime_ = *reinterpret_cast<double*>(pkgs_[i].content);
            stopIndex_ = static_cast<unsigned int>(i);
            break;
        }
    }
}

bool IsObjAvailableInEntities(const std::vector<ScenarioEntities> entities, int id)
{
    bool status = false;
    if (entities.size() == 0)
    {
        return status = false;
    }
    for (size_t i = 0; i < entities.size(); i++)
    {
        if (entities[i].obj_id == id)
        {
            status = true;
            break;
        }
        else if ( i == entities.size() - 1) // last iteration
        {
            status = false;
            break;
        }
    }
    return status;
}

void Replay::GetScenarioEntities()
{

    double time = 0.0;
    int id = -1;
    for (size_t i = 0; i < pkgs_.size(); i++)
    {
        if (static_cast<datLogger::PackageId>(pkgs_[i].hdr.id) == datLogger::PackageId::TIME_SERIES)
        {
            time = *reinterpret_cast<double*>(pkgs_[i].content);
        }

        if (static_cast<datLogger::PackageId>(pkgs_[i].hdr.id) == datLogger::PackageId::OBJ_ID)
        {
            id = *reinterpret_cast<int*>(pkgs_[i].content);
            if(!IsObjAvailableInEntities(entities, id))
            {
                ScenarioEntities entities_;
                entities_.sim_time = time;
                entities_.obj_id = id;
                entities.push_back(entities_);
            }
        }
    }
}

double Replay::GetTimeFromEntities(int id)
{
    double t = SMALL_NUMBER;
    for (size_t i = 0; i < entities.size(); i++)
    {
        if (entities[i].obj_id ==  id)
        {
            t = entities[i].sim_time;
            break;
        }
    }
    return t;
}

double Replay::GetTimeFromCnt(int count)
{
    double timeTemp = -1.0;
    int    count_   = 0;
    for (size_t i = 0; i < pkgs_.size(); i++)  // return time if idx is already time pkg or find the respective time pkg for given pkg idx
    {
        if (static_cast<datLogger::PackageId>(pkgs_[i].hdr.id) == datLogger::PackageId::TIME_SERIES)  // find time pkg for given pkg idx
        {
            count_ += 1;
            if (count == count_)
            {
                timeTemp = *reinterpret_cast<double*>(pkgs_[i].content);
                break;
            }
        }
    }
    return timeTemp;
}

void Replay::AddObjState(size_t idx, double t)
{
    ObjectStateWithObjId stateObjId;
    if (static_cast<datLogger::PackageId>(pkgs_[idx].hdr.id) != datLogger::PackageId::OBJ_ID)
    {
        std::cout << " Initialization error->Stop replay " << std::endl;
    }
    stateObjId.id = *reinterpret_cast<int*>(pkgs_[idx].content);
    stateObjId.active = true;
    int pkgCount  = GetPkgCntBtwObj(idx);

    for (size_t i = idx + 1; i < static_cast<size_t>(pkgCount) + idx + 1; i++)
    {  // GetPkgCntBtwObj will return count of package

        if (datLogger::PackageId::OBJ_ADDED == static_cast<datLogger::PackageId>(pkgs_[i].hdr.id) ||
            datLogger::PackageId::OBJ_DELETED == static_cast<datLogger::PackageId>(pkgs_[i].hdr.id))
            {
                continue; // skip packages.
            }
        ObjectStateWithPkg statePkg;
        statePkg.pkg   = reinterpret_cast<char*>(&pkgs_[i]);
        statePkg.time_ = t;
        stateObjId.pkgs.push_back(statePkg);
    }
    scenarioState.obj_states.push_back(stateObjId);
}

void  Replay::deleteObjState(int objId)
{
    for (size_t i = 0; i < scenarioState.obj_states.size(); i++) // loop current state object id to find the object id
    {
        if (scenarioState.obj_states[i].id == objId) // found object id
        {
            scenarioState.obj_states[i].active = false;
            // scenarioState.obj_states.erase(scenarioState.obj_states.begin() + static_cast<unsigned int>(i));
        }
    }
}

void Replay::InitiateStates()
{
    std::vector<int> objIdIndices = GetNumberOfObjectsAtTime();

    if (objIdIndices.size() == 0)
    {
        // no obj found for given time, may be given time frame pkg not available.
        std::cout << " No obj found for given time, may be given time frame pkg not available. " << std::endl;
    }

    scenarioState.sim_time = startTime_;
    for (size_t Index = 0; Index < objIdIndices.size(); Index++)
    {
        AddObjState(static_cast<size_t>(objIdIndices[Index]), startTime_);
    }
}

int Replay::GetModelID(int obj_id)
{
    int model_id = -1;
    for (size_t i = 0; i < scenarioState.obj_states.size(); i++)
    {
        if (scenarioState.obj_states[i].id != obj_id)
        {
            continue;
        }
        for (size_t j = 0; j < scenarioState.obj_states[i].pkgs.size(); j++)
        {
            datLogger::CommonPkg* pkg;
            pkg = reinterpret_cast<datLogger::CommonPkg*>(scenarioState.obj_states[i].pkgs[j].pkg);
            if (static_cast<datLogger::PackageId>(pkg->hdr.id) == datLogger::PackageId::MODEL_ID)
            {
                model_id = *reinterpret_cast<int*>(pkg->content);
            }
        }
    }
    return model_id;
}

int Replay::GetCtrlType(int obj_id)
{
    int ctrl_type = -1;
    for (size_t i = 0; i < scenarioState.obj_states.size(); i++)
    {
        if (scenarioState.obj_states[i].id != obj_id)
        {
            continue;
        }
        for (size_t j = 0; j < scenarioState.obj_states[i].pkgs.size(); j++)
        {
            datLogger::CommonPkg* pkg;
            pkg = reinterpret_cast<datLogger::CommonPkg*>(scenarioState.obj_states[i].pkgs[j].pkg);
            if (static_cast<datLogger::PackageId>(pkg->hdr.id) == datLogger::PackageId::CTRL_TYPE)
            {
                ctrl_type = *reinterpret_cast<int*>(pkg->content);
            }
        }
    }
    return ctrl_type;
}

int Replay::GetBB(int obj_id, OSCBoundingBox& bb)
{
    datLogger::BoundingBox bb_;
    for (size_t i = 0; i < scenarioState.obj_states.size(); i++)
    {
        if (scenarioState.obj_states[i].id != obj_id)
        {
            continue;
        }
        for (size_t j = 0; j < scenarioState.obj_states[i].pkgs.size(); j++)
        {
            datLogger::CommonPkg* pkg;
            pkg = reinterpret_cast<datLogger::CommonPkg*>(scenarioState.obj_states[i].pkgs[j].pkg);
            if (static_cast<datLogger::PackageId>(pkg->hdr.id) == datLogger::PackageId::BOUNDING_BOX)
            {
                bb_ = *reinterpret_cast<datLogger::BoundingBox*>(pkg->content);
                break;
            }
        }
    }
    bb.center_.x_ = bb_.x;
    bb.center_.y_ = bb_.y;
    bb.center_.z_ = bb_.z;
    bb.dimensions_.height_ = bb_.height;
    bb.dimensions_.length_ = bb_.length;
    bb.dimensions_.width_ = bb_.width;

    return 0;
}

int Replay::GetScaleMode(int obj_id)
{
    int scale_mode = -1;
    for (size_t i = 0; i < scenarioState.obj_states.size(); i++)
    {
        if (scenarioState.obj_states[i].id != obj_id)
        {
            continue;
        }
        for (size_t j = 0; j < scenarioState.obj_states[i].pkgs.size(); j++)
        {
            datLogger::CommonPkg* pkg;
            pkg = reinterpret_cast<datLogger::CommonPkg*>(scenarioState.obj_states[i].pkgs[j].pkg);
            if (static_cast<datLogger::PackageId>(pkg->hdr.id) == datLogger::PackageId::SCALE_MODE)
            {
               scale_mode = *reinterpret_cast<int*>(pkg->content);
            }
        }
    }

    return scale_mode;
}

int Replay::GetVisibility(int obj_id)
{
    int vis = -1;
    for (size_t i = 0; i < scenarioState.obj_states.size(); i++)
    {
        if (scenarioState.obj_states[i].id != obj_id)
        {
            continue;
        }
        for (size_t j = 0; j < scenarioState.obj_states[i].pkgs.size(); j++)
        {
            datLogger::CommonPkg* pkg;
            pkg = reinterpret_cast<datLogger::CommonPkg*>(scenarioState.obj_states[i].pkgs[j].pkg);
            if (static_cast<datLogger::PackageId>(pkg->hdr.id) == datLogger::PackageId::VISIBILITY_MASK)
            {
               vis = *reinterpret_cast<int*>(pkg->content);
            }
        }
    }

    return vis;
}

datLogger::Pos Replay::GetPos(int obj_id)
{
    datLogger::Pos pos;
    for (size_t i = 0; i < scenarioState.obj_states.size(); i++)
    {
        if (scenarioState.obj_states[i].id != obj_id)
        {
            continue;
        }
        for (size_t j = 0; j < scenarioState.obj_states[i].pkgs.size(); j++)
        {
            datLogger::CommonPkg* pkg;
            pkg = reinterpret_cast<datLogger::CommonPkg*>(scenarioState.obj_states[i].pkgs[j].pkg);
            if (static_cast<datLogger::PackageId>(pkg->hdr.id) == datLogger::PackageId::POSITIONS)
            {
               pos = *reinterpret_cast<datLogger::Pos*>(pkg->content);
            }
        }
    }
    return pos;
}

double Replay::GetX(int obj_id)
{
    datLogger::Pos pos = GetPos(obj_id);
    return pos.x;
}

double Replay::GetY(int obj_id)
{
    datLogger::Pos pos = GetPos(obj_id);
    return pos.y;
}

double Replay::GetZ(int obj_id)
{
    datLogger::Pos pos = GetPos(obj_id);
    return pos.z;
}

double Replay::GetH(int obj_id)
{
    datLogger::Pos pos = GetPos(obj_id);
    return pos.h;
}

double Replay::GetR(int obj_id)
{
    datLogger::Pos pos = GetPos(obj_id);
    return pos.r;
}

double Replay::GetP(int obj_id)
{
    datLogger::Pos pos = GetPos(obj_id);
    return pos.r;
}

int Replay::GetRoadId(int obj_id)
{
    int road_id = -1;
    for (size_t i = 0; i < scenarioState.obj_states.size(); i++)
    {
        if (scenarioState.obj_states[i].id != obj_id)
        {
            continue;
        }
        for (size_t j = 0; j < scenarioState.obj_states[i].pkgs.size(); j++)
        {
            datLogger::CommonPkg* pkg;
            pkg = reinterpret_cast<datLogger::CommonPkg*>(scenarioState.obj_states[i].pkgs[j].pkg);
            if (static_cast<datLogger::PackageId>(pkg->hdr.id) == datLogger::PackageId::ROAD_ID)
            {
               road_id = *reinterpret_cast<int*>(pkg->content);
            }
        }
    }

    return road_id;
}

int Replay::GetLaneId(int obj_id)
{
    int lane_id = -1;
    for (size_t i = 0; i < scenarioState.obj_states.size(); i++)
    {
        if (scenarioState.obj_states[i].id != obj_id)
        {
            continue;
        }
        for (size_t j = 0; j < scenarioState.obj_states[i].pkgs.size(); j++)
        {
            datLogger::CommonPkg* pkg;
            pkg = reinterpret_cast<datLogger::CommonPkg*>(scenarioState.obj_states[i].pkgs[j].pkg);
            if (static_cast<datLogger::PackageId>(pkg->hdr.id) == datLogger::PackageId::LANE_ID)
            {
               lane_id = *reinterpret_cast<int*>(pkg->content);
            }
        }
    }

    return lane_id;
}

double Replay::GetPosOffset(int obj_id)
{
    double offset = 0.0;
    for (size_t i = 0; i < scenarioState.obj_states.size(); i++)
    {
        if (scenarioState.obj_states[i].id != obj_id)
        {
            continue;
        }
        for (size_t j = 0; j < scenarioState.obj_states[i].pkgs.size(); j++)
        {
            datLogger::CommonPkg* pkg;
            pkg = reinterpret_cast<datLogger::CommonPkg*>(scenarioState.obj_states[i].pkgs[j].pkg);
            if (static_cast<datLogger::PackageId>(pkg->hdr.id) == datLogger::PackageId::POS_OFFSET)
            {
               offset = *reinterpret_cast<double*>(pkg->content);
            }
        }
    }

    return offset;
}

float Replay::GetPosT(int obj_id)
{
    double pos_t = 0.0;
    for (size_t i = 0; i < scenarioState.obj_states.size(); i++)
    {
        if (scenarioState.obj_states[i].id != obj_id)
        {
            continue;
        }
        for (size_t j = 0; j < scenarioState.obj_states[i].pkgs.size(); j++)
        {
            datLogger::CommonPkg* pkg;
            pkg = reinterpret_cast<datLogger::CommonPkg*>(scenarioState.obj_states[i].pkgs[j].pkg);
            if (static_cast<datLogger::PackageId>(pkg->hdr.id) == datLogger::PackageId::POS_T)
            {
               pos_t = *reinterpret_cast<double*>(pkg->content);
            }
        }
    }

    return static_cast<float>(pos_t);
}

float Replay::GetPosS(int obj_id)
{
    double pos_s = 0.0;
    for (size_t i = 0; i < scenarioState.obj_states.size(); i++)
    {
        if (scenarioState.obj_states[i].id != obj_id)
        {
            continue;
        }
        for (size_t j = 0; j < scenarioState.obj_states[i].pkgs.size(); j++)
        {
            datLogger::CommonPkg* pkg;
            pkg = reinterpret_cast<datLogger::CommonPkg*>(scenarioState.obj_states[i].pkgs[j].pkg);
            if (static_cast<datLogger::PackageId>(pkg->hdr.id) == datLogger::PackageId::POS_S)
            {
               pos_s = *reinterpret_cast<double*>(pkg->content);
            }
        }
    }

    return static_cast<float>(pos_s);
}

ObjectPositionStructDat Replay::GetComPletePos(int obj_id)
{
    ObjectPositionStructDat complete_pos;
    datLogger::Pos pos;
    pos = GetPos(obj_id);
    complete_pos.h = static_cast<float>(pos.h);
    complete_pos.x = static_cast<float>(pos.x);
    complete_pos.y = static_cast<float>(pos.y);
    complete_pos.z = static_cast<float>(pos.z);
    complete_pos.p = static_cast<float>(pos.p);
    complete_pos.r = static_cast<float>(pos.r);
    complete_pos.laneId = GetLaneId(obj_id);
    complete_pos.roadId = GetRoadId(obj_id);
    complete_pos.offset = static_cast<float>(GetPosOffset(obj_id));
    complete_pos.t = GetPosT(obj_id);
    complete_pos.s = GetPosS(obj_id);
    return complete_pos;
}

double Replay::GetWheelAngle(int obj_id)
{
    double wheel_angle = 0.0;
    for (size_t i = 0; i < scenarioState.obj_states.size(); i++)
    {
        if (scenarioState.obj_states[i].id != obj_id)
        {
            continue;
        }
        for (size_t j = 0; j < scenarioState.obj_states[i].pkgs.size(); j++)
        {
            datLogger::CommonPkg* pkg;
            pkg = reinterpret_cast<datLogger::CommonPkg*>(scenarioState.obj_states[i].pkgs[j].pkg);
            if (static_cast<datLogger::PackageId>(pkg->hdr.id) == datLogger::PackageId::WHEEL_ANGLE)
            {
               wheel_angle = *reinterpret_cast<double*>(pkg->content);
            }
        }
    }

    return wheel_angle;
}

double Replay::GetWheelRot(int obj_id)
{
    double wheel_rot = 0.0;
    for (size_t i = 0; i < scenarioState.obj_states.size(); i++)
    {
        if (scenarioState.obj_states[i].id != obj_id)
        {
            continue;
        }
        for (size_t j = 0; j < scenarioState.obj_states[i].pkgs.size(); j++)
        {
            datLogger::CommonPkg* pkg;
            pkg = reinterpret_cast<datLogger::CommonPkg*>(scenarioState.obj_states[i].pkgs[j].pkg);
            if (static_cast<datLogger::PackageId>(pkg->hdr.id) == datLogger::PackageId::WHEEL_ROT)
            {
               wheel_rot = *reinterpret_cast<double*>(pkg->content);
            }
        }
    }

    return wheel_rot;
}

double Replay::GetSpeed(int obj_id)
{
    double speed = 0.0;
    for (size_t i = 0; i < scenarioState.obj_states.size(); i++)
    {
        if (scenarioState.obj_states[i].id != obj_id)
        {
            continue;
        }
        for (size_t j = 0; j < scenarioState.obj_states[i].pkgs.size(); j++)
        {
            datLogger::CommonPkg* pkg;
            pkg = reinterpret_cast<datLogger::CommonPkg*>(scenarioState.obj_states[i].pkgs[j].pkg);
            if (static_cast<datLogger::PackageId>(pkg->hdr.id) == datLogger::PackageId::SPEED)
            {
               speed = *reinterpret_cast<double*>(pkg->content);
            }
        }
    }

    return speed;
}

int Replay::GetName(int obj_id, std::string& name)
{
    for (size_t i = 0; i < scenarioState.obj_states.size(); i++)
    {
        if (scenarioState.obj_states[i].id != obj_id)
        {
            continue;
        }
        for (size_t j = 0; j < scenarioState.obj_states[i].pkgs.size(); j++)
        {
            datLogger::CommonPkg* pkg;
            pkg = reinterpret_cast<datLogger::CommonPkg*>(scenarioState.obj_states[i].pkgs[j].pkg);
            if (static_cast<datLogger::PackageId>(pkg->hdr.id) == datLogger::PackageId::NAME)
            {
                name = reinterpret_cast<datLogger::Name*>(pkg->content)->string;
            }
        }
    }
    return 0;
}

void Replay::SetStartTime(double time)
{
    startTime_ = time;
    startIndex_ = static_cast<unsigned int>(FindIndexAtTimestamp(startTime_));
    if (time_ < startTime_)
    {
        time_ = startTime_;
        index_ = startIndex_;
    }
}

void Replay::SetStopTime(double time)
{
    stopTime_ = time;
    stopIndex_ = static_cast<unsigned int>(FindIndexAtTimestamp(stopTime_));
    if (time_ > stopTime_)
    {
        time_ = stopTime_;
        index_ = stopIndex_;
    }
}

void Replay::CleanEntries(std::vector<ReplayEntry>& entries)
{
    for (unsigned int i = 0; i < entries.size() - 1; i++)
    {
        if (entries[i + 1].state.info.timeStamp < entries[i].state.info.timeStamp)
        {
            entries.erase(entries.begin() + i + 1);
            i--;
        }

        for (unsigned int j = 1; (i + j < entries.size()) && NEAR_NUMBERSF(entries[i + j].state.info.timeStamp, entries[i].state.info.timeStamp); j++)
        {
            // Keep the latest instance of entries with same timestamp
            if (entries[i + j].state.info.id == entries[i].state.info.id)
            {
                entries.erase(entries.begin() + i);
                i--;
                break;
            }
        }
    }
}

void Replay::BuildData(std::vector<std::pair<std::string, std::vector<ReplayEntry>>>& scenarios)
{
    // Keep track of current index of each scenario
    std::vector<int> cur_idx;
    std::vector<int> next_idx;

    for (size_t j = 0; j < scenarios.size(); j++)
    {
        cur_idx.push_back(0);
        next_idx.push_back(0);
    }

    // Set scenario ID-group (0, 100, 200 etc.)
    for (size_t j = 0; j < scenarios.size(); j++)
    {
        for (size_t k = 0; k < scenarios[j].second.size(); k++)
        {
            // Set scenario ID-group (0, 100, 200 etc.)
            scenarios[j].second[k].state.info.id += static_cast<int>(j) * 100;
        }
    }

    // Populate data_ based on first (with lowest timestamp) scenario
    double cur_timestamp = static_cast<double>(scenarios[0].second[0].state.info.timeStamp);
    while (cur_timestamp < LARGE_NUMBER - SMALL_NUMBER)
    {
        // populate entries if all scenarios at current time step
        double min_time_stamp = LARGE_NUMBER;
        for (size_t j = 0; j < scenarios.size(); j++)
        {
            if (next_idx[j] != -1)
            {
                unsigned int k = static_cast<unsigned int>(cur_idx[j]);
                for (; k < scenarios[j].second.size() && static_cast<double>(scenarios[j].second[k].state.info.timeStamp) < cur_timestamp + 1e-6; k++)
                {
                    // push entry with modified timestamp
                    scenarios[j].second[k].state.info.timeStamp = static_cast<float>(cur_timestamp);
                    data_.push_back(scenarios[j].second[k]);
                }

                if (k < scenarios[j].second.size())
                {
                    next_idx[j] = static_cast<int>(k);
                    if (static_cast<double>(scenarios[j].second[k].state.info.timeStamp) < min_time_stamp)
                    {
                        min_time_stamp = static_cast<double>(scenarios[j].second[k].state.info.timeStamp);
                    }
                }
                else
                {
                    next_idx[j] = -1;
                }
            }
        }

        if (min_time_stamp < LARGE_NUMBER - SMALL_NUMBER)
        {
            for (size_t j = 0; j < scenarios.size(); j++)
            {
                if (next_idx[j] > 0 && static_cast<double>(scenarios[j].second[static_cast<unsigned int>(next_idx[j])].state.info.timeStamp) <
                                           min_time_stamp + SMALL_NUMBER)
                {
                    // time has reached next entry, step this scenario
                    cur_idx[j] = next_idx[j];
                }
            }
        }

        cur_timestamp = min_time_stamp;
    }
}

void Replay::CreateMergedDatfile(const std::string filename)
{
    std::ofstream data_file_;
    data_file_.open(filename, std::ofstream::binary);
    if (data_file_.fail())
    {
        LOG("Cannot open file: %s", filename.c_str());
        exit(-1);
    }

    data_file_.write(reinterpret_cast<char*>(&header_), sizeof(header_));

    if (data_file_.is_open())
    {
        // Write status to file - for later replay
        for (size_t i = 0; i < data_.size(); i++)
        {
            data_file_.write(reinterpret_cast<char*>(&data_[i].state), sizeof(data_[i].state));
        }
    }
}