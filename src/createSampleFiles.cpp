#include "createSampleFiles.h"

#include "boost/filesystem.hpp"
namespace bfs = boost::filesystem;
using namespace std;

void load_pgm_images_from_directory(std::string dir, xstd::mvector<2, cv::Mat>& imgVector){
    size_t nPatches = 9;
    cv::Size imageSize = cv::Size(48, 48);
    cv::Size patchCounts = cv::Size(3, 3);
    bfs::directory_iterator endDir;

    if (bfs::is_directory(dir))
    {
        for (bfs::directory_iterator itDir(dir); itDir != endDir; ++itDir)
        {
            if (bfs::is_regular_file(*itDir) && itDir->path().extension() == ".png")
            {
                size_t neg = imgVector.size();
                imgVector.push_back(xstd::mvector<1, cv::Mat>(nPatches));
                std::vector<cv::Mat> patches = imPreprocess(itDir->path().string(), imageSize, patchCounts,
                                                            ESVM_USE_HISTOGRAM_EQUALIZATION, "WINDOW_NAME", cv::IMREAD_GRAYSCALE);
                for (size_t p = 0; p < nPatches; p++)
                    imgVector[neg][p] = patches[p];
            }                  
        }
    }
}

int create_negatives()
{
    #if PROC_ESVM_GENERATE_SAMPLE_FILES

    logstream logger("negatives-output.txt");
    
    // outputs
    bool writeBinaryFormat = true;
    bool writeLibsvmFormat = false;
    std::string windowNameOriginal = "WINDOW_ORIGINAL";     // display oringal 'cropped_face' ROIs
    std::string windowNameROI = "WINDOW_ROI";               // display localized ROIs from LBP improved (if activated)
    int delayShowROI = 1;                                   // [ms] - show found LBP improved ROIs with a delay (for visual inspection)
    bool keepAllFoundROI = false;                           // keep all the found ROIs (if applicable), or only the first one (if applicable)
    assert(writeBinaryFormat || writeLibsvmFormat);
    assert(delayShowROI > 0);

    // general parameters
    bool useRefineROI = true;                               // enable LBP improved localized ROI refinement
    size_t nPatches = 9;
    cv::Size patchCounts = cv::Size(3, 3);
    cv::Size imageSize = cv::Size(48, 48);

    // improved LBP face detection parameters
    // (try to focus roi on more descriptive part of the face)
    double scaleFactor = 1.1;
    int nmsThreshold = 1;                           // 0 will generate multiple detections, >0 usually returns only 1 face on 'cropped_faces' ROIs
    cv::Size minSize(20, 20), maxSize = imageSize;
    cv::CascadeClassifier faceCascade;
    if (useRefineROI) {
        std::string faceCascadeFilePath = sourcesOpenCV + "data/lbpcascades/lbpcascade_frontalface_improved.xml";
        assert(bfs::is_regular_file(faceCascadeFilePath));
        assert(faceCascade.load(faceCascadeFilePath));
    }

    // feature extraction HOG parameters
    cv::Size patchSize = cv::Size(imageSize.width / patchCounts.width, imageSize.height / patchCounts.height);
    cv::Size blockSize = cv::Size(2, 2);
    cv::Size blockStride = cv::Size(2, 2);
    cv::Size cellSize = cv::Size(2, 2);
    int nBins = 3;

    // Negatives to employ
    std::vector<std::string> negativesID = { "0001", "0002", "0007", "0009", "0011",
                                             "0013", "0014", "0016", "0017", "0018",
                                             "0019", "0020", "0021", "0022", "0025" };

    // init containers / classes with parameters
    xstd::mvector<2, cv::Mat> matNegativeSamples;               // [negative][patch](Mat[x,y])
    std::vector<std::string> negativeSamplesID;                 // [negative](string)
    xstd::mvector<2, FeatureVector> fvNegativeSamples;          // [patch][negative](FeatureVector)
    FeatureExtractorHOG hog;
    hog.initialize(patchSize, blockSize, blockStride, cellSize, nBins);

    // Loop for all ChokePoint cropped faces
    std::vector<int> perSessionNegatives(SESSION_QUANTITY, 0);
    std::vector<int> perSequenceNegatives(PORTAL_QUANTITY * SESSION_QUANTITY * PORTAL_TYPE_QUANTITY * CAMERA_QUANTITY, 0);
    int seqIdx = 0;
    std::vector<PORTAL_TYPE> types = { ENTER, LEAVE };
    bfs::directory_iterator endDir;
    for (int sn = 1; sn <= SESSION_QUANTITY; sn++) {
    for (int pn = 1; pn <= PORTAL_QUANTITY; pn++) {
    for (auto it = types.begin(); it != types.end(); ++it) {
    for (int cn = 1; cn <= CAMERA_QUANTITY; cn++)
    {     
        string seq = buildChokePointSequenceString(pn, *it, sn, cn);

        // Add ROI to corresponding sample vectors according to individual IDs            
        for (int id = 1; id <= INDIVIDUAL_QUANTITY; id++)
        {
            std::string dirPath = roiChokePointCroppedFacePath + buildChokePointSequenceString(pn, *it, sn, cn, id) + "/";
            logger << "Loading negative from directory: '" << dirPath << "'" << std::endl;
            if (bfs::is_directory(dirPath))
            {
                for (bfs::directory_iterator itDir(dirPath); itDir != endDir; ++itDir)
                {
                    if (bfs::is_regular_file(*itDir) && itDir->path().extension() == ".pgm")
                    {
                        std::string strID = buildChokePointIndividualID(id);
                        if (contains(negativesID, strID))
                        {
                            size_t neg = matNegativeSamples.size();
                            matNegativeSamples.push_back(xstd::mvector<1, cv::Mat>(nPatches));
                            std::string imgPath = itDir->path().string();
                            cv::Mat img = cv::imread(imgPath, cv::IMREAD_GRAYSCALE);
                            cv::Mat roi;
                            std::vector<cv::Rect> faces;
                            if (useRefineROI)
                            {
                                cv::imshow(windowNameOriginal, img);
                                cv::waitKey(1);
                                faceCascade.detectMultiScale(img, faces, scaleFactor, nmsThreshold, cv::CASCADE_SCALE_IMAGE, minSize, maxSize);
                                size_t nFaces = faces.size();
                                if (nFaces > 0)
                                {
                                    logger << "Found " << faces.size() << " face(s)" << std::endl;
                                    for (size_t iFace = 0; iFace < nFaces; ++iFace) {
                                        if (keepAllFoundROI || iFace == 0)              // update kept ROI according to setting
                                            roi = img(faces[iFace]);
                                        logger << "  face[" << iFace << "] = " << faces[iFace] << std::endl;
                                        cv::imshow(windowNameROI, img(faces[iFace]));   // always display all found ROIs
                                        cv::waitKey(delayShowROI);
                                    }
                                }
                                else
                                {
                                    logger << "Did not find face on cropped image: '" << imgPath << "'" << std::endl;
                                    continue;   // skip if not found any face
                                }
                            }
                            std::vector<cv::Mat> patches = imPreprocess(useRefineROI ? roi : img, imageSize, patchCounts,
                                                                        ESVM_USE_HISTOGRAM_EQUALIZATION, windowNameROI, cv::IMREAD_GRAYSCALE);
                            for (size_t p = 0; p < nPatches; p++)
                                matNegativeSamples[neg][p] = patches[p];

                            negativeSamplesID.push_back(strID);
                            perSessionNegatives[sn-1]++;
                            perSequenceNegatives[seqIdx]++;
                        }
                    }                  
                }
            }                        
        }            
        seqIdx++;

    } } } } // end ChokePoint loops

    cv::destroyAllWindows();

    size_t nNegatives = matNegativeSamples.size();
    size_t dimsNegatives[2] = { nPatches, nNegatives };                     // [patch][negative]
    fvNegativeSamples = xstd::mvector<2, FeatureVector>(dimsNegatives);

    // feature extraction HOG
    for (size_t p = 0; p < nPatches; p++)
        for (size_t neg = 0; neg < nNegatives; neg++)
            fvNegativeSamples[p][neg] = hog.compute(matNegativeSamples[neg][p]);

    // find + apply normalization values
    double minAllROI = DBL_MAX, maxAllROI = -DBL_MAX, meanAllROI = -DBL_MAX, stdDevAllROI = -DBL_MAX;
    xstd::mvector<2, FeatureVector> fvNegativeSamplesMinMaxPatch(dimsNegatives), fvNegativeSamplesMinMaxROI(dimsNegatives),
                                    fvNegativeSamplesZScorePatch(dimsNegatives), fvNegativeSamplesZScoreROI(dimsNegatives);
    std::vector<double> fvMinPatch(nPatches, DBL_MAX), fvMaxPatch(nPatches, -DBL_MAX), fvMeanPatch(nPatches), fvStdDevPatch(nPatches);
    for (size_t p = 0; p < nPatches; p++)
    {
        findNormParamsOverall(MIN_MAX, fvNegativeSamples[p], &fvMinPatch[p], &fvMaxPatch[p]);
        findNormParamsOverall(Z_SCORE, fvNegativeSamples[p], &fvMeanPatch[p], &fvStdDevPatch[p]);
        logger << "Patch Number: " << p << " Min: " << fvMinPatch[p] << " Max: " << fvMaxPatch[p]
               << " Mean: " << fvMeanPatch[p] << " StdDev: " << fvStdDevPatch[p] << std::endl;
        
        for (size_t neg = 0; neg < nNegatives; neg++) {
            fvNegativeSamplesMinMaxPatch[p][neg] = normalizeAllFeatures(MIN_MAX, fvNegativeSamples[p][neg], fvMinPatch[p], fvMaxPatch[p]);
            fvNegativeSamplesZScorePatch[p][neg] = normalizeAllFeatures(Z_SCORE, fvNegativeSamples[p][neg], fvMeanPatch[p], fvStdDevPatch[p], true);
        }
        if (minAllROI > fvMinPatch[p])
            minAllROI = fvMinPatch[p];
        if (maxAllROI < fvMaxPatch[p])
            maxAllROI = fvMaxPatch[p];
    }

    std::vector<FeatureVector> fvNegativeSamplesAllPatches;
    fvNegativeSamplesAllPatches.reserve(nPatches * nNegatives);
    for (size_t p = 0; p < nPatches; p++)
        fvNegativeSamplesAllPatches.insert(fvNegativeSamplesAllPatches.end(), fvNegativeSamples[p].begin(), fvNegativeSamples[p].end());
    findNormParamsOverall(Z_SCORE, fvNegativeSamplesAllPatches, &meanAllROI, &stdDevAllROI);

    for (size_t p = 0; p < nPatches; p++)
        for (size_t neg = 0; neg < nNegatives; neg++) {
            fvNegativeSamplesMinMaxROI[p][neg] = normalizeAllFeatures(MIN_MAX, fvNegativeSamples[p][neg], minAllROI, maxAllROI);
            fvNegativeSamplesZScoreROI[p][neg] = normalizeAllFeatures(Z_SCORE, fvNegativeSamples[p][neg], meanAllROI, stdDevAllROI, true);
        }

    // write resulting sample files
    std::vector<int> negClass(nNegatives, ESVM_NEGATIVE_CLASS);
    for (size_t p = 0; p < nPatches; p++)
    {
        std::string fileStart = "negatives-patch" + std::to_string(p);
        if (writeBinaryFormat) {
            ESVM::writeSampleDataFile(fileStart + "-raw.bin",               fvNegativeSamples[p],            negClass, BINARY);
            ESVM::writeSampleDataFile(fileStart + "-normPatch-minmax.bin",  fvNegativeSamplesMinMaxPatch[p], negClass, BINARY);
            ESVM::writeSampleDataFile(fileStart + "-normROI-minmax.bin",    fvNegativeSamplesMinMaxROI[p],   negClass, BINARY);
            ESVM::writeSampleDataFile(fileStart + "-normPatch-zcore.bin",   fvNegativeSamplesZScorePatch[p], negClass, BINARY);
            ESVM::writeSampleDataFile(fileStart + "-normROI-zcore.bin",     fvNegativeSamplesZScoreROI[p],   negClass, BINARY);
        }
        if (writeLibsvmFormat) {
            ESVM::writeSampleDataFile(fileStart + "-raw.data",              fvNegativeSamples[p],            negClass, LIBSVM);
            ESVM::writeSampleDataFile(fileStart + "-normPatch-minmax.data", fvNegativeSamplesMinMaxPatch[p], negClass, LIBSVM);
            ESVM::writeSampleDataFile(fileStart + "-normROI-minmax.data",   fvNegativeSamplesMinMaxROI[p],   negClass, LIBSVM);
            ESVM::writeSampleDataFile(fileStart + "-normPatch-zcore.data",  fvNegativeSamplesZScorePatch[p], negClass, LIBSVM);
            ESVM::writeSampleDataFile(fileStart + "-normROI-zcore.data",    fvNegativeSamplesZScoreROI[p],   negClass, LIBSVM);
        }
    }

    // write configs employed (traceback)
    logstream logSampleConfig("negatives-output-config.txt");
    logSampleConfig << "negativeIDs:      " << negativesID << std::endl
                    << "nNegatives:       " << nNegatives << std::endl
                    << "perSessionNeg:    " << perSessionNegatives << std::endl
                    << "perSeqNeg         " << perSequenceNegatives << std::endl
                    << "histEqual:        " << ESVM_USE_HISTOGRAM_EQUALIZATION << std::endl
                    << "useRefineROI:     " << useRefineROI << std::endl
                    << "scaleFactor:      " << scaleFactor << std::endl
                    << "nmsThreshold:     " << nmsThreshold << std::endl
                    << "CC minSize:       " << minSize << std::endl
                    << "CC maxSize:       " << maxSize << std::endl
                    << "imageSize:        " << imageSize << std::endl
                    << "nPatches:         " << nPatches << std::endl
                    << "patchCounts:      " << patchCounts << std::endl
                    << "patchSize:        " << patchSize << std::endl
                    << "blockSize:        " << blockSize << std::endl
                    << "blockStride:      " << blockStride << std::endl
                    << "cellSize:         " << cellSize << std::endl
                    << "nBins:            " << nBins << std::endl                    
                    << "fv minAll[p]:     " << fvMinPatch << std::endl
                    << "fv maxAll[p]:     " << fvMaxPatch << std::endl
                    << "fv minAllROI:     " << minAllROI << std::endl
                    << "fv maxAllROI:     " << maxAllROI << std::endl
                    << "fv meanAll[p]:    " << fvMeanPatch << std::endl
                    << "fv stdDevAll[p]:  " << fvStdDevPatch << std::endl
                    << "fv meanAllROI:    " << meanAllROI << std::endl
                    << "fv stdDevAllROI:  " << stdDevAllROI << std::endl
                    << "nFeatures:        " << fvNegativeSamples[0][0].size() << std::endl
                    << "BINARY fmt?:      " << writeBinaryFormat << std::endl
                    << "LIBSVM fmt?:      " << writeLibsvmFormat << std::endl
                    << "all Neg IDs:      " << negativeSamplesID << std::endl;

    #else/*PROC_ESVM_GENERATE_SAMPLE_FILES*/
    return passThroughDisplayTestStatus(__func__, SKIPPED);
    #endif/*PROC_ESVM_GENERATE_SAMPLE_FILES*/
    return passThroughDisplayTestStatus(__func__, NO_ERROR);
}

int create_probes(std::string positives, std::string negatives)
{
    logstream logger("probes-output.txt");

    double hogHardcodedFoundMin = 0;            // Min found using 'FullChokePoint' test with SAMAN pre-generated files
    double hogHardcodedFoundMax = 0.675058;     // Max found using 'FullChokePoint' test with SAMAN pre-generated files
    size_t nPatches = 9;
    xstd::mvector<2, cv::Mat> matPositiveSamples, matNegativeSamples;
    std::vector<PORTAL_TYPE> types = { ENTER, LEAVE };
    cv::Size patchCounts = cv::Size(3, 3);
    cv::Size imageSize = cv::Size(48, 48);
    xstd::mvector<2, FeatureVector> fvNegativeSamples, fvPositiveSamples;                   // [patch][descriptor][negative](FeatureVector)

    // Hog init
    cv::Size patchSize = cv::Size(imageSize.width / patchCounts.width, imageSize.height / patchCounts.height);
    cv::Size blockSize = cv::Size(2, 2);
    cv::Size blockStride = cv::Size(2, 2);
    cv::Size cellSize = cv::Size(2, 2);
    int nBins = 3;
    FeatureExtractorHOG hog;
    hog.initialize(patchSize, blockSize, blockStride, cellSize, nBins);

    // ChokePoint Path
    std::string rootChokePointPath = std::string(std::getenv("CHOKEPOINT_ROOT")) + "/";         // ChokePoint dataset root
    std::string roiChokePointCroppedFacePath = rootChokePointPath + "cropped_faces/";           // Path of extracted 96x96 ROI from all videos 

    // Add ROI to corresponding sample vectors according to individual IDs
    logger << "Loading probe images for sequence " << positives << "...: " << std::endl;
    load_pgm_images_from_directory(positives, matPositiveSamples);
    logger << "Loading probe images for sequence " << negatives << "...: " << std::endl;
    load_pgm_images_from_directory(negatives, matNegativeSamples);

    size_t nPositives = matPositiveSamples.size();
    size_t nNegatives = matNegativeSamples.size();

    size_t dims[2] = { nPatches, 0 };                             // [patch][negative/positive]
    fvNegativeSamples = xstd::mvector<2, FeatureVector>(dims);
    fvPositiveSamples = xstd::mvector<2, FeatureVector>(dims);

    // Calculate Feature Vectors
    for (size_t p = 0; p < nPatches; p++){
        for (size_t pos = 0; pos < nPositives; pos++)
            fvPositiveSamples[p].push_back(hog.compute(matPositiveSamples[pos][p]));
        for (size_t neg = 0; neg < nNegatives; neg++)
            fvNegativeSamples[p].push_back(hog.compute(matNegativeSamples[neg][p]));
    }

    for (size_t p = 0; p < nPatches; p++){
        for (size_t pos = 0; pos < nPositives; pos++)
            fvPositiveSamples[p][pos] = normalizeAllFeatures(MIN_MAX, fvPositiveSamples[p][pos], hogHardcodedFoundMin, hogHardcodedFoundMax);
        for (size_t neg = 0; neg < nNegatives; neg++)
            fvNegativeSamples[p][neg] = normalizeAllFeatures(MIN_MAX, fvNegativeSamples[p][neg], hogHardcodedFoundMin, hogHardcodedFoundMax);
    }

    for (size_t p = 0; p < nPatches; p++)
        fvPositiveSamples[p].insert(fvPositiveSamples[p].end(), fvNegativeSamples[p].begin(), fvNegativeSamples[p].end());

    std::vector<int> targetOutputs(nPositives, 1);
    std::vector<int> targetOutputsNeg(nNegatives, -1);
    targetOutputs.insert(targetOutputs.end(), targetOutputsNeg.begin(), targetOutputsNeg.end());

    logger << "Size check - pos: " << targetOutputs.size() << " neg: " << targetOutputsNeg.size() << std::endl;

    for (size_t p = 0; p < nPatches; p++)
        ESVM::writeSampleDataFile("ID0003-probes-hog-patch" + std::to_string(p) + ".bin", fvPositiveSamples[p], targetOutputs, BINARY);

    // ofstream outputFile;
    // outputFile.open ("example1.txt");

    // logger << "nNegatives: " << nNegatives << " nPatches: " << nPatches << endl;
    // for (size_t p = 0; p < nPatches; p++)
    //     for (size_t neg = 0; neg < nNegatives; neg++){
    //         for (size_t i = 0; i < fvNegativeSamples[p][neg].size(); i++){
    //             outputFile << fvNegativeSamples[p][neg][i];
    //         }
    //         outputFile << endl;
    //     }

    // outputFile.close();

    logger << "DONE!" << std::endl;
    return 0;
}
