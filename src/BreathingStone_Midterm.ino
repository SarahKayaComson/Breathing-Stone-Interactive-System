/*
  Project: Breathing Stone (숨 쉬는 돌) - Midterm Prototype
  Author: Hwang Sang-yeon (Comson)
  Description: HCI-based Adaptive Biofeedback System for Meditation.
*/

#define SENSOR_PIN A0  // 광학식 PPG 심박 센서 입력 핀
#define VIB_PIN 9      // 진동 스피커 및 앰프(PAM8403) 출력을 위한 PWM 핀 [cite: 419]

// 1. 이동평균 필터(Moving Average Filter) 변수 
const int numReadings = 10; // 10개의 샘플 데이터 평균 
int readings[numReadings];
int readIndex = 0;
long total = 0;
int average = 0;

// 2. 실시간 BPM 연산 변수
unsigned long lastBeatTime = 0;
int currentBPM = 0;
const int threshold = 550; // 환경에 맞게 영점(Threshold) 조절 필요
boolean belowThreshold = true;

// 3. '3분 골든타임' 및 페이드아웃 변수 [cite: 406, 438]
unsigned long stableStartTime = 0;
boolean isStable = false;
const unsigned long goldenTime = 180000;  // 180초(3분)를 밀리초로 환산 [cite: 406]
const unsigned long fadeDuration = 10000; // 10초에 걸친 선형적 페이드아웃 [cite: 439]

// 4. 1/f 자연음 합성(Procedural Generation) 변수 [cite: 413, 414]
float lowPassNoise = 0.0;

void setup() {
  Serial.begin(115200); // 시리얼 플로터 통신 속도
  pinMode(VIB_PIN, OUTPUT);
  
  // 배열 초기화
  for (int i = 0; i < numReadings; i++) {
    readings[i] = 0;
  }
}

void loop() {
  unsigned long currentMillis = millis();

  // ---------------------------------------------------------
  // [Step 1] 이동평균 필터 기반 데이터 정제 (Baseline Drift 제거) [cite: 400, 401]
  // ---------------------------------------------------------
  total = total - readings[readIndex];
  readings[readIndex] = analogRead(SENSOR_PIN);
  total = total + readings[readIndex];
  readIndex = (readIndex + 1) % numReadings;
  average = total / numReadings; // 정제된 평균값 산출 [cite: 402]

  // ---------------------------------------------------------
  // [Step 2] 실시간 BPM(Beats Per Minute) 연산
  // ---------------------------------------------------------
  if (average > threshold && belowThreshold) {
    unsigned long beatDuration = currentMillis - lastBeatTime;
    currentBPM = 60000 / beatDuration;
    lastBeatTime = currentMillis;
    belowThreshold = false;
  } else if (average < threshold - 20) {
    belowThreshold = true;
  }

  // ---------------------------------------------------------
  // [Step 3] 안정 상태 판별 및 타이머 제어 [cite: 405, 437]
  // ---------------------------------------------------------
  if (currentBPM <= 60 && currentBPM > 30) { 
    if (!isStable) {
      isStable = true;
      stableStartTime = currentMillis; // 60 BPM 이하 진입 시 타이머 시작 [cite: 405]
    }
  } else {
    isStable = false; // 불안정 상태(60 BPM 초과)일 경우 개입 시작 [cite: 434]
  }

  // ---------------------------------------------------------
  // [Step 4] 사운드 및 햅틱 출력 레벨(Fade) 계산 [cite: 406, 438]
  // ---------------------------------------------------------
  float outputLevel = 0.0;
  
  if (!isStable || (isStable && (currentMillis - stableStartTime < goldenTime))) {
    outputLevel = 1.0; // 불안정하거나 3분이 지나지 않았을 때는 100% 개입
  } else if (isStable && (currentMillis - stableStartTime >= goldenTime)) {
    // 3분이 경과한 시점부터 선형적 페이드아웃 (Linear Fade-out) [cite: 438]
    unsigned long fadeElapsed = (currentMillis - stableStartTime) - goldenTime;
    if (fadeElapsed < fadeDuration) {
      outputLevel = 1.0 - ((float)fadeElapsed / fadeDuration); // 서서히 감소 [cite: 438]
    } else {
      outputLevel = 0.0; // 최종적으로 진동 세기가 0에 수렴 [cite: 439]
    }
  }

  // ---------------------------------------------------------
  // [Step 5] 서브 베이스 사인파 + 1/f 노이즈 합성 및 PWM 출력 [cite: 411, 412, 413]
  // ---------------------------------------------------------
  if (outputLevel > 0) {
    // 30Hz 대역의 촉각적 뼈대(서브 베이스 사인파) 생성 [cite: 411]
    float sineVal = sin(currentMillis * 2.0 * PI * 30.0 / 1000.0);

    // random() 함수와 로우패스 필터를 활용한 1/f 특성 노이즈 합성 (자연음 텍스처) [cite: 413, 434]
    float whiteNoise = random(-255, 255) / 255.0; 
    lowPassNoise = 0.9 * lowPassNoise + 0.1 * whiteNoise; 

    // 두 파형을 결합하고 PWM 출력값(0~255)으로 스케일링 [cite: 406]
    float combinedSignal = (sineVal * 0.5) + (lowPassNoise * 0.5); 
    int pwmValue = (int)((combinedSignal + 1.0) * 127.5 * outputLevel); 

    analogWrite(VIB_PIN, constrain(pwmValue, 0, 255)); // 9번 핀으로 출력 [cite: 419]
  } else {
    analogWrite(VIB_PIN, 0); // 개입 소거 시 0 출력
  }

  // 시리얼 플로터 확인용 데이터 출력 (Raw vs Filtered) [cite: 427, 430]
  Serial.print("Raw:");
  Serial.print(readings[readIndex == 0 ? numReadings - 1 : readIndex - 1]);
  Serial.print(",Filtered:");
  Serial.println(average);

  delay(10); // 약 100Hz 샘플링 속도 유지
}
