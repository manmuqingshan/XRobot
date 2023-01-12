#include "dev_imu.hpp"

#include "webots/accelerometer.h"
#include "webots/gyro.h"
#include "webots/inertial_unit.h"

using namespace Device;

IMU::IMU(IMU::Param& param)
    : param_(param),
      accl_tp_((param.tp_name_prefix + std::string("_accl")).c_str()),
      gyro_tp_((param.tp_name_prefix + std::string("_gyro")).c_str()),
      eulr_tp_((param.tp_name_prefix + std::string("_eulr")).c_str()) {
  this->ahrs_handle_ = wb_robot_get_device("imu");
  this->gyro_handle_ = wb_robot_get_device("gyro");
  this->accl_handle_ = wb_robot_get_device("accl");

  wb_inertial_unit_enable(this->ahrs_handle_, 1000);
  wb_accelerometer_enable(this->accl_handle_, 1000);
  wb_gyro_enable(this->gyro_handle_, 1000);

  auto thread_fn = [](IMU* imu) {
    while (1) {
      imu->accl_tp_.Publish(imu->accl_);
      imu->gyro_tp_.Publish(imu->gyro_);
      imu->eulr_tp_.Publish(imu->eulr_);

      imu->Update();

      imu->thread_.SleepUntil(1);
    }
  };

  this->thread_.Create(thread_fn, this, "imu", 512, System::Thread::Realtime);
}

void IMU::Update() {
  const double* accl_data = wb_accelerometer_get_values(this->accl_handle_);
  const double* gyro_data = wb_gyro_get_values(this->gyro_handle_);
  const double* eulr_data =
      wb_inertial_unit_get_roll_pitch_yaw(this->ahrs_handle_);

  this->eulr_.rol = eulr_data[0];
  this->eulr_.pit = eulr_data[1];
  this->eulr_.yaw = eulr_data[2];

  this->gyro_.x = gyro_data[0];
  this->gyro_.y = gyro_data[1];
  this->gyro_.z = gyro_data[2];

  this->accl_.x = accl_data[0];
  this->accl_.y = accl_data[1];
  this->accl_.z = accl_data[2];
}