// blink_detector.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "BlinkDetector.h"
#include "HumanDetector.h"

//std::string path = "tbd";

namespace ark {
	BlinkDetector::BlinkDetector(DetectionParams::Ptr params) {
		// Since we have seen no humans previously, we set this to default value
		BlinkDetector::total = 0;
		BlinkDetector::ear = 0;
		BlinkDetector::facemark = cv::face::FacemarkLBF::create();
		facemark->loadModel(HumanDetector::FACE_LBFMODEL_FILE_PATH);
		faceDetector.load(HumanDetector::FACE_HAARCASCADE_FILE_PATH);

	}

	void BlinkDetector::visualizeBlink(DepthCamera::Ptr & camera, cv::Mat & rgbMap){
		for (int k = 0; k < l_eye_pts.size(); k++) {
			cv::circle(rgbMap, l_eye_pts[k], 1, cv::Scalar(0, 0, 255));
		}

		for (int l = 0; l < r_eye_pts.size(); l++) {
			cv::circle(rgbMap, r_eye_pts[l], 1, cv::Scalar(0, 0, 255));
		}
		char blink_str[20], ear_str[20];
		sprintf(blink_str, "Total # blinks: %i\n", BlinkDetector::total);
		sprintf(ear_str, "EAR: %.2f\n", BlinkDetector::ear);
		cv::putText(rgbMap, blink_str, cv::Point2f(10, 30),
			cv::FONT_HERSHEY_SIMPLEX, 0.7, (0, 0, 255), 2);
		cv::putText(rgbMap, ear_str, cv::Point2f(300, 30),
			cv::FONT_HERSHEY_SIMPLEX, 0.7, (0, 0, 255), 2);
		cv::imshow(camera->getModelName() + " RGB", rgbMap);
	}

	float BlinkDetector::getEyeAspectRatio(std::vector<cv::Point2d> eye) {
		float p2p6 = cv::norm(eye[1] - eye[5]);
		float p3p5 = cv::norm(eye[2] - eye[4]);
		float p1p4 = cv::norm(eye[0] - eye[3]);

		float ear = (p2p6 + p3p5) / (2.0 * p1p4); // EAR formula suggested by Cech 2016.
		return ear;
	}

	void BlinkDetector::update(cv::Mat &rgbMap) {
		detect(rgbMap);
	}

	void BlinkDetector::detect(cv::Mat &image) {
		detectBlink(image);
	}

	int BlinkDetector::getTotal() {
		return BlinkDetector::total;
	}

	float BlinkDetector::getEar() {
		return BlinkDetector::ear;
	}

	std::vector<cv::Point2d> BlinkDetector::getLeftEyePts () {
		return BlinkDetector::l_eye_pts;
	}

	std::vector<cv::Point2d> BlinkDetector::getRightEyePts() {
		return BlinkDetector::r_eye_pts;
	}

	void BlinkDetector::detectBlink(cv::Mat &rgbMap) {
		BlinkDetector::l_eye_pts.clear();
		BlinkDetector::r_eye_pts.clear();

		cv::Mat gray;
		std::vector<cv::Rect> faces;
		std::vector<std::vector<cv::Point2f>> landmarks;

		cv::cvtColor(rgbMap, gray, cv::COLOR_BGR2GRAY);
		faceDetector.detectMultiScale(gray, faces);
		if (!(facemark->fit(gray, faces, landmarks))) {
			return;
		}
		
		if (landmarks[0].size() == 68) {
			for (int i = 36; i < 42; i++) {
				BlinkDetector::l_eye_pts.push_back(landmarks[0][i]);
			}
			for (int j = 42; j < 48; j++) {
				BlinkDetector::r_eye_pts.push_back(landmarks[0][j]);
			}
		}
		else {
			cout << "HUH\n" << endl;
		}
		float left_EAR = BlinkDetector::getEyeAspectRatio(l_eye_pts);
		float right_EAR = BlinkDetector::getEyeAspectRatio(r_eye_pts);
		BlinkDetector::ear = (left_EAR + right_EAR) / 2.0;
		std::cout << "Eye aspect ratio: " << BlinkDetector::ear << std::endl;
		if (BlinkDetector::ear < EYE_AR_THRESH) {
			BlinkDetector::consecBlinkCounter++;
		}
		else {
			if (BlinkDetector::consecBlinkCounter >= EYE_AR_CONSEC_FRAMES) {
				BlinkDetector::total++;
			}
			BlinkDetector::consecBlinkCounter = 0;
		}
	}
}