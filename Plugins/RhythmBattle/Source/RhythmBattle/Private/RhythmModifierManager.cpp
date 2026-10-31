#include "RhythmModifierManager.h"

bool URhythmModifierManager::Add(const FRhythmModifierSpec& Spec, double SongTime, double SecondsPerBeat)
{
    if (Spec.Id.IsNone() || !FMath::IsFinite(Spec.Value) || Spec.Value <= 0.f ||
        !FMath::IsFinite(Spec.DurationSeconds) || Spec.DurationSeconds <= 0. ||
        !FMath::IsFinite(SongTime) || !FMath::IsFinite(SecondsPerBeat) || SecondsPerBeat <= 0.) return false;
    RemoveModifier(Spec.Id);
    Active.Add(Spec);
    Expiry.Add(Spec.Id, SongTime + Spec.DurationSeconds * (Spec.bDurationInBeats ? SecondsPerBeat : 1.));
    return true;
}
bool URhythmModifierManager::RemoveModifier(FName Id)
{
    Expiry.Remove(Id);
    return Active.RemoveAll([Id](const FRhythmModifierSpec& S) { return S.Id == Id; }) > 0;
}
void URhythmModifierManager::Expire(double SongTime)
{
    for (int32 I = Active.Num()-1; I >= 0; --I)
        if (SongTime >= Expiry.FindChecked(Active[I].Id)) RemoveModifier(Active[I].Id);
}
void URhythmModifierManager::Reset() { Active.Reset(); Expiry.Reset(); }
float URhythmModifierManager::Evaluate(ERhythmModifierType Type, int32 Player, int32 Lane) const
{
    double Result = Type == ERhythmModifierType::LaneDisable ? 0. : 1.;
    for (const FRhythmModifierSpec& S : Active)
    {
        if (S.Type != Type || (S.Scope != ERhythmModifierScope::Battle && S.PlayerIndex != Player) ||
            (S.Scope == ERhythmModifierScope::Lane && S.LaneId != Lane)) continue;
        if (Type == ERhythmModifierType::LaneDisable) return 1.f;
        Result = FMath::Clamp(Result * S.Value, 0.01, 100.);
    }
    return static_cast<float>(Result);
}
TArray<FRhythmModifierSpec> URhythmModifierManager::GetActiveModifiers() const { return Active; }
