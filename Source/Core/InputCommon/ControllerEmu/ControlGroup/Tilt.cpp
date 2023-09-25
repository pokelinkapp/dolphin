// Copyright 2017 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "InputCommon/ControllerEmu/ControlGroup/Tilt.h"

#include <string>

#include "Common/Common.h"
#include "Common/MathUtil.h"

#include "InputCommon/ControlReference/ControlReference.h"
#include "InputCommon/ControllerEmu/Control/Control.h"
#include "InputCommon/ControllerEmu/Control/Input.h"
#include "InputCommon/ControllerEmu/Setting/NumericSetting.h"

namespace ControllerEmu
{
Tilt::Tilt(const std::string& name_) : ReshapableInput(name_, name_, GroupType::Tilt)
{
  AddInput(Translatability::Translate, _trans("Forward"));
  AddInput(Translatability::Translate, _trans("Backward"));
  AddInput(Translatability::Translate, _trans("Left"));
  AddInput(Translatability::Translate, _trans("Right"));

  AddInput(Translatability::Translate, _trans("Modifier"));

  AddSetting(&m_max_angle_setting,
             {_trans("Angle"),
              // i18n: The symbol/abbreviation for degrees (unit of angular measure).
              _trans("°"),
              // i18n: Refers to tilting an emulated Wii Remote.
              _trans("Maximum tilt angle.")},
             85, 0, 180);

  AddSetting(&m_max_rotational_velocity,
             {_trans("Velocity"),
              // i18n: The symbol/abbreviation for hertz (cycles per second).
              _trans("Hz"),
              // i18n: Refers to tilting an emulated Wii Remote.
              _trans("Peak angular velocity (measured in turns per second).")},
             7, 1, 50);
}

Tilt::ReshapeData Tilt::GetReshapableState(bool adjusted) const
{
  const ControlState y = controls[0]->GetState() - controls[1]->GetState();
  const ControlState x = controls[3]->GetState() - controls[2]->GetState();

  // Return raw values. (used in UI)
  if (!adjusted)
    return {x, y};

  return Reshape(x, y, GetModifierInput()->GetState());
}

Tilt::StateData Tilt::GetState(const ControllerEmu::InputOverrideFunction& override_func) const
{
  Tilt::StateData state = GetReshapableState(true);

  if (!override_func)
    return state;

  if (const std::optional<ControlState> x_override = override_func(name, X_INPUT_OVERRIDE, state.x))
    state.x = *x_override;
  if (const std::optional<ControlState> y_override = override_func(name, Y_INPUT_OVERRIDE, state.y))
    state.y = *y_override;

  return state;
}

ControlState Tilt::GetGateRadiusAtAngle(double ang, const ControllerEmu::InputOverrideFunction& override_func) const
{
  ControlState max_tilt_angle = m_max_angle_setting.GetValue() / 180;

  if (!override_func)
    return SquareStickGate(max_tilt_angle).GetRadiusAtAngle(ang);

  if (const std::optional<ControlState> angle_override = override_func(name, ANGLE, max_tilt_angle))
    max_tilt_angle = *angle_override;

  return SquareStickGate(max_tilt_angle).GetRadiusAtAngle(ang);
}

ControlState Tilt::GetGateRadiusAtAngle(double ang) const
{
  ControlState max_tilt_angle = m_max_angle_setting.GetValue() / 180;

  return SquareStickGate(max_tilt_angle).GetRadiusAtAngle(ang);
}

ControlState Tilt::GetDefaultInputRadiusAtAngle(double ang) const
{
  return SquareStickGate(1.0).GetRadiusAtAngle(ang);
}

ControlState Tilt::GetMaxRotationalVelocity(const ControllerEmu::InputOverrideFunction& override_func) const
{
  ControlState max_rotational_velocity = m_max_rotational_velocity.GetValue() * MathUtil::TAU;

  if (!override_func)
    return max_rotational_velocity;

  if (const std::optional<ControlState> velocity_override = override_func(name, VELOCITY, max_rotational_velocity))
    max_rotational_velocity = *velocity_override;

  return max_rotational_velocity;
}

Control* Tilt::GetModifierInput() const
{
  return controls[4].get();
}

}  // namespace ControllerEmu
