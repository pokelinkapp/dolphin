// Copyright 2017 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <string>

#include "Common/Matrix.h"
#include "InputCommon/ControllerEmu/StickGate.h"

namespace ControllerEmu
{
class Force : public ReshapableInput
{
public:
  using StateData = Common::Vec3;

  explicit Force(const std::string& name);

  ReshapeData GetReshapableState(bool adjusted) const final override;
  ControlState GetGateRadiusAtAngle(double ang) const final override;

  ControlState GetDefaultInputRadiusAtAngle(double angle) const final override;

  StateData GetState(bool adjusted, const ControllerEmu::InputOverrideFunction& override_func) const;

  // Velocities returned in m/s.
  ControlState GetSpeed(const ControllerEmu::InputOverrideFunction& override_func) const;
  ControlState GetReturnSpeed(const ControllerEmu::InputOverrideFunction& override_func) const;

  // Return twist angle in radians.
  ControlState GetTwistAngle(const ControllerEmu::InputOverrideFunction& override_func) const;

  // Return swing distance in meters.
  ControlState GetMaxDistance(const ControllerEmu::InputOverrideFunction& override_func) const;

  static constexpr const char* DISTANCE = "Distance";
  static constexpr const char* SPEED = "Speed";
  static constexpr const char* RETURN_SPEED = "Return Speed";
  static constexpr const char* ANGLE = "Angle";

private:
  SettingValue<double> m_distance_setting;
  SettingValue<double> m_speed_setting;
  SettingValue<double> m_return_speed_setting;
  SettingValue<double> m_angle_setting;
};

class Shake : public ControlGroup
{
public:
  using StateData = Common::Vec3;

  explicit Shake(const std::string& name, ControlState default_intensity_scale = 1);

  StateData GetState(bool adjusted, const ControllerEmu::InputOverrideFunction& override_func) const;

  ControlState GetDeadzone() const;

  // Return total travel distance in meters.
  ControlState GetIntensity(const ControllerEmu::InputOverrideFunction& override_func) const;

  // Return frequency in Hz.
  ControlState GetFrequency(const ControllerEmu::InputOverrideFunction& override_func) const;

  static constexpr const char* INTENSITY = "Intensity";
  static constexpr const char* FREQUENCY = "Frequency";

private:
  SettingValue<double> m_deadzone_setting;
  SettingValue<double> m_intensity_setting;
  SettingValue<double> m_frequency_setting;
};

}  // namespace ControllerEmu
