// Copyright 2017 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "InputCommon/ControllerEmu/ControlGroup/Force.h"

#include <string>

#include "Common/Common.h"
#include "Common/MathUtil.h"

#include "InputCommon/ControlReference/ControlReference.h"
#include "InputCommon/ControllerEmu/Control/Input.h"
#include "InputCommon/ControllerEmu/ControllerEmu.h"
#include "InputCommon/ControllerEmu/Setting/NumericSetting.h"

namespace ControllerEmu
{
Force::Force(const std::string& name_) : ReshapableInput(name_, name_, GroupType::Force)
{
  AddInput(Translatability::Translate, _trans("Up"));
  AddInput(Translatability::Translate, _trans("Down"));
  AddInput(Translatability::Translate, _trans("Left"));
  AddInput(Translatability::Translate, _trans("Right"));
  AddInput(Translatability::Translate, _trans("Forward"));
  AddInput(Translatability::Translate, _trans("Backward"));

  AddSetting(&m_distance_setting,
             {_trans("Distance"),
              // i18n: The symbol/abbreviation for centimeters.
              _trans("cm"),
              // i18n: Refering to emulated wii remote swing movement.
              _trans("Distance of travel from neutral position.")},
             50, 1, 100);

  // These speed settings are used to calculate a maximum jerk (change in acceleration).
  // The calculation uses a travel distance of 1 meter.
  // The maximum value of 40 m/s is the approximate speed of the head of a golf club.
  // Games seem to not even properly detect motions at this speed.
  // Values result in an exponentially increasing jerk.

  AddSetting(&m_speed_setting,
             {_trans("Speed"),
              // i18n: The symbol/abbreviation for meters per second.
              _trans("m/s"),
              // i18n: Refering to emulated wii remote swing movement.
              _trans("Peak velocity of outward swing movements.")},
             16, 1, 40);

  // "Return Speed" allows for a "slow return" that won't trigger additional actions.
  AddSetting(&m_return_speed_setting,
             {_trans("Return Speed"),
              // i18n: The symbol/abbreviation for meters per second.
              _trans("m/s"),
              // i18n: Refering to emulated wii remote swing movement.
              _trans("Peak velocity of movements to neutral position.")},
             2, 1, 40);

  AddSetting(&m_angle_setting,
             {_trans("Angle"),
              // i18n: The symbol/abbreviation for degrees (unit of angular measure).
              _trans("°"),
              // i18n: Refering to emulated wii remote swing movement.
              _trans("Rotation applied at extremities of swing.")},
             90, 1, 180);
}

Force::ReshapeData Force::GetReshapableState(bool adjusted) const
{
  const ControlState y = controls[0]->GetState() - controls[1]->GetState();
  const ControlState x = controls[3]->GetState() - controls[2]->GetState();

  // Return raw values. (used in UI)
  if (!adjusted)
    return {x, y};

  return Reshape(x, y);
}

Force::StateData Force::GetState(bool adjusted, const ControllerEmu::InputOverrideFunction& override_func) const
{
  auto state = GetReshapableState(adjusted);
  ControlState z = controls[4]->GetState() - controls[5]->GetState();

  if (adjusted)
  {
    // Apply deadzone to z and scale.
    z = ApplyDeadzone(z, GetDeadzonePercentage()) * GetMaxDistance(override_func);
  }

  if (!override_func)
    return {float(state.x), float(state.y), float(z)};

  if (const std::optional<ControlState> x_dir_override = override_func(name, X_INPUT_OVERRIDE, state.x))
    state.x = *x_dir_override;
  if (const std::optional<ControlState> y_dir_override = override_func(name, Y_INPUT_OVERRIDE, state.y))
    state.y = *y_dir_override;
  if (const std::optional<ControlState> z_dir_override = override_func(name, Z_INPUT_OVERRIDE, z))
    z = *z_dir_override;

  return {float(state.x), float(state.y), float(z)};
}

ControlState Force::GetGateRadiusAtAngle(double) const
{
  // Just a circle of the configured distance:
  return GetMaxDistance(nullptr);
}

ControlState Force::GetSpeed(const ControllerEmu::InputOverrideFunction& override_func) const
{
  ControlState speed = m_speed_setting.GetValue();
  if (!override_func)
    return speed;

  if (const std::optional<ControlState> speed_override = override_func(name, SPEED, speed))
    speed = *speed_override;

  return speed;
}

ControlState Force::GetReturnSpeed(const ControllerEmu::InputOverrideFunction& override_func) const
{
  ControlState return_speed = m_return_speed_setting.GetValue();
  if (!override_func)
    return return_speed;

  if (const std::optional<ControlState> return_speed_override = override_func(name, RETURN_SPEED, return_speed))
    return_speed = *return_speed_override;

  return return_speed;
}

ControlState Force::GetTwistAngle(const ControllerEmu::InputOverrideFunction& override_func) const
{
  ControlState angle = m_angle_setting.GetValue() * MathUtil::TAU / 360;
  if (!override_func)
    return angle;

  if (const std::optional<ControlState> angle_override = override_func(name, ANGLE, angle))
    angle = *angle_override;

  return angle;

}

ControlState Force::GetMaxDistance(const ControllerEmu::InputOverrideFunction& override_func) const
{
  ControlState distance = m_distance_setting.GetValue() / 100;
  if (!override_func)
    return distance;

  if (const std::optional<ControlState> distance_override = override_func(name, DISTANCE, distance))
    distance = *distance_override;

  return distance;
}

ControlState Force::GetDefaultInputRadiusAtAngle(double) const
{
  // Just a circle of radius 1.0.
  return 1.0;
}

Shake::Shake(const std::string& name_, ControlState default_intensity_scale)
    : ControlGroup(name_, name_, GroupType::Shake)
{
  // i18n: Refers to a 3D axis (used when mapping motion controls)
  AddInput(Translatability::Translate, _trans("X"));
  // i18n: Refers to a 3D axis (used when mapping motion controls)
  AddInput(Translatability::Translate, _trans("Y"));
  // i18n: Refers to a 3D axis (used when mapping motion controls)
  AddInput(Translatability::Translate, _trans("Z"));

  AddDeadzoneSetting(&m_deadzone_setting, 50);

  // Total travel distance in centimeters.
  // Negative values can be used to reverse the initial direction of movement.
  AddSetting(&m_intensity_setting,
             // i18n: Refers to the intensity of shaking an emulated wiimote.
             {_trans("Intensity"),
              // i18n: The symbol/abbreviation for centimeters.
              _trans("cm"),
              // i18n: Refering to emulated wii remote movement.
              _trans("Total travel distance.")},
             10 * default_intensity_scale, -50, 50);

  // Approximate number of up/down movements in one second.
  AddSetting(&m_frequency_setting,
             // i18n: Refers to a number of actions per second in Hz.
             {_trans("Frequency"),
              // i18n: The symbol/abbreviation for hertz (cycles per second).
              _trans("Hz"),
              // i18n: Refering to emulated wii remote movement.
              _trans("Number of shakes per second.")},
             6, 1, 20);
}

Shake::StateData Shake::GetState(bool adjusted, const ControllerEmu::InputOverrideFunction& override_func) const
{
  ControlState x = controls[0]->GetState();
  ControlState y = controls[1]->GetState();
  ControlState z = controls[2]->GetState();

  StateData result = {float(x), float(y), float(z)};

  // FYI: Unadjusted values are used in UI.
  if (adjusted)
    for (auto& c : result.data)
      c = ApplyDeadzone(c, GetDeadzone());

  if (!override_func)
    return result;

  if (const std::optional<ControlState> x_override = override_func(name, Force::X_INPUT_OVERRIDE, x))
    x = *x_override;
  if (const std::optional<ControlState> y_override = override_func(name, Force::Y_INPUT_OVERRIDE, y))
    y = *y_override;
  if (const std::optional<ControlState> z_override = override_func(name, Force::Z_INPUT_OVERRIDE, z))
    z = *z_override;

  return {float(x), float(y), float(z)};
}

ControlState Shake::GetDeadzone() const
{
  return m_deadzone_setting.GetValue() / 100;
}

ControlState Shake::GetIntensity(const ControllerEmu::InputOverrideFunction& override_func) const
{
  ControlState intensity = m_intensity_setting.GetValue() / 100;

  if (!override_func)
    return intensity;

  if (const std::optional<ControlState> intensity_override = override_func(name, INTENSITY, intensity))
    intensity = *intensity_override;

  return intensity;
}

ControlState Shake::GetFrequency(const ControllerEmu::InputOverrideFunction& override_func) const
{
  ControlState frequency = m_frequency_setting.GetValue();

  if (!override_func)
    return frequency;

  if (const std::optional<ControlState> frequency_override = override_func(name, FREQUENCY, frequency))
    frequency = *frequency_override;

  return frequency;
}

}  // namespace ControllerEmu
