/*
  Project: Breathing Stone - v1.1 (Async & Exponential Fade)
  Author: Hwang Sang-yeon (Comson)
  Description: 3D 프린팅 기구부 결합을 고려한 비동기 심박 처리 및 1/f 햅틱 피드백.
               (1) delay() 제거 및 millis() 기반 Non-blocking 아키텍처 적용
               (2) 선형 감쇄에서 지수형(Exponential) 감쇄로 자연스러운 페이드아웃 구현
               (3) 3D 출력물 틈새를 위한 호흡형(Breathing) LED 시각 효과 추가
*/

#define SENSOR_PIN A0  
#define VIB_PIN 9      
#define LED_PIN 11     // PWM 제어를 위해 13번에서 11번 핀으로 변경

// --- 시스템 타이머 변수 ---
unsigned long previousMillis = 0;
const long samplingInterval = 10; // 100Hz 샘플링

// --- 심박(PPG) 센서 및 필터 변수 ---
const int numReadings = 10;
int readings[numReadings];
int readIndex = 0;
long total = 0;
int average = 0;
const int threshold = 550; 
boolean belowThreshold = true;

// --- BPM 및 햅틱 상태 변수 ---
unsigned long lastBeatTime = 0;
int currentBPM = 0;
float lowPassNoise = 0.0;

// --- 페이드아웃(안정화) 변수 ---
unsigned long stableStartTime = 0;
boolean isStable = false;
float fadeMultiplier = 1.0; 

void setup() {
  Serial.begin(115200);
  pinMode(VIB_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  
  for (int i = 0; i < numReadings; i++) {
    readings[i] = 0;
  }
}

void loop() {
  unsigned long currentMillis = millis();

  // 10ms 마다 비동기적으로 센서 샘플링 및 필터링 수행
  if (currentMillis - previousMillis >= samplingInterval) {
    previousMillis = currentMillis;

    // [모듈 1] 이동평균 필터 연산
    total = total - readings[readIndex];
    readings[readIndex] = analogRead(SENSOR_PIN);
    total = total + readings[readIndex];
    readIndex = (readIndex + 1) % numReadings;
    average = total / numReadings;

    // [모듈 2] 피크 감지 및 BPM 연산
    if (average > threshold && belowThreshold) {
      unsigned long beatDuration = currentMillis - lastBeatTime;
      
      if (beatDuration > 300) { // 노이즈 방어 (Max 200 BPM)
        currentBPM = 60000 / beatDuration;
        lastBeatTime = currentMillis;
      }
      belowThreshold = false;
      
    } else if (average < threshold - 20) {
      belowThreshold = true;
    }

    // [모듈 3] 상태 판단 및 지수형 페이드아웃 연산
    if (currentBPM <= 60 && currentBPM > 40) {
      if (!isStable) {
        isStable = true;
        stableStartTime = currentMillis;
      }
      // 안정 상태 진입 후 180초 동안 지수형으로 진동 감쇄 (자립 유도)
      unsigned long stableDuration = currentMillis - stableStartTime;
      if (stableDuration < 180000) {
        // e^(-x) 형태의 자연스러운 감쇄 곡선 적용
        fadeMultiplier = exp(-5.0 * (float)stableDuration / 180000.0);
      } else {
        fadeMultiplier = 0.0; // 3분 후 기계적 개입 완전 소거
      }
    } else {
      isStable = false;
      fadeMultiplier = 1.0; // 불안정 상태 시 피드백 100% 복구
    }

    // [모듈 4] 햅틱 출력 (1/f 노이즈) 및 LED 브리딩 동기화
    if (fadeMultiplier > 0.01) {
      float sineVal = sin(currentMillis * 2.0 * PI * 30.0 / 1
