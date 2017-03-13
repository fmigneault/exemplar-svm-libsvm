#ifndef ESVM_TESTS_H
#define ESVM_TESTS_H

#include "opencv2/opencv.hpp"
#include "mvector.hpp"
#include "esvmTypes.h"
#include "esvm.h"

/* ChokePoint Dataset:
      P#T_S#_C#:      2 portals, 2 types (E:enter/L:leave), 4 sessions, 3 cameras = 48 video dirs (with Ground Truths, one by one individuals pass)
      P2T_S5_C#:      portal #2 only, 2 types (E:enter/L:leave), session #5, 3 cameras = 6 video dirs (no Ground Truths, simultaneous individuals)
      Crop Face dir:  up to 30 individuals (if present) with N face ROIs
*/
// Possible sequence information
enum PORTAL_TYPE { ENTER, LEAVE };
const int PORTAL_TYPE_QUANTITY = 2;
const int PORTAL_QUANTITY = 2;
const int SESSION_QUANTITY = 4;
const int SESSION_SIMULTANEOUS_INDIVIDUALS = 5;
const int CAMERA_QUANTITY = 3;
const int INDIVIDUAL_QUANTITY = 30;
// Combine sequence information
std::string buildChokePointSequenceString(int portal, PORTAL_TYPE type, int session, int camera, int id = 0);
std::string buildChokePointIndividualID(int id, bool withPrefixID = false);

/* Test utilities */
svm_node buildNode(int index, double value);
svm_model buildDummyExemplarSvmModel();
bool checkPathEndSlash(std::string path);

/* Utilities */
void generateDummySamples(std::vector<FeatureVector>& samples, std::vector<int>& targetOutputs, size_t nSamples, size_t nFeatures);
bool generateDummySampleFile_libsvm(std::string filePath, size_t nSamples, size_t nFeatures);
bool generateDummySampleFile_binary(std::string filePath, size_t nSamples, size_t nFeatures);

/* Tests */
int test_outputOptions();
int test_imagePaths();
int test_imagePatchExtraction();
int test_imagePreprocessing();
int test_multiLevelVectors();
int test_normalizationFunctions();
int test_performanceEvaluationFunctions();
int test_ESVM_BasicFunctionalities();
int test_ESVM_BasicClassification();
int test_ESVM_ReadSampleFile_libsvm();
int test_ESVM_ReadSampleFile_binary();
int test_ESVM_ReadSampleFile_compare();
int test_ESVM_ReadSampleFile_timing(size_t nSamples, size_t nFeatures);
int test_ESVM_WriteSampleFile_timing(size_t nSamples, size_t nFeatures);
int test_ESVM_SaveLoadModelFile_libsvm();
int test_ESVM_SaveLoadModelFile_binary();
int test_ESVM_SaveLoadModelFile_compare();
int test_runSingleSamplePerPersonStillToVideo(cv::Size patchCounts);
int test_runSingleSamplePerPersonStillToVideo_FullChokePoint(cv::Size imageSize, cv::Size patchCounts);
int test_runSingleSamplePerPersonStillToVideo_DataFiles_WholeImage();
int test_runSingleSamplePerPersonStillToVideo_DataFiles_DescriptorAndPatchBased(size_t nPatches);
int test_runSingleSamplePerPersonStillToVideo_NegativesDataFiles_PositivesExtraction_PatchBased();
int test_runSingleSamplePerPersonStillToVideo_TITAN(cv::Size imageSize, cv::Size patchCounts, bool useSyntheticPositives);
int test_runSingleSamplePerPersonStillToVideo_DataFiles_SAMAN();
int test_runSingleSamplePerPersonStillToVideo_DataFiles_SimplifiedWorkingProcedure();

/* Performance Evaluation */
void eval_PerformanceClassificationScores(std::vector<double> normScores, std::vector<int> probeGroundTruths);
void eval_PerformanceClassificationScores(std::vector<double> normScores, std::vector<int> probeGroundTruths, 
                                          std::vector<double>& FPR, std::vector<double>& TPR);
void eval_PerformanceClassificationSummary(std::vector<std::string> positivesID,
                                           xstd::mvector<2, double> normScores, xstd::mvector<2, int> probeGroundTruths);

#endif/*ESVM_TESTS_H*/