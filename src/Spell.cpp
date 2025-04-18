#include "Spell.h"

Spell::Spell(std::string name, SpellType type, float damage, float fireRate, float range, float cooldown, SpellType weakness)
    : name(name), type(type), damage(damage), fireRate(fireRate), range(range), cooldown(cooldown), weakness(weakness) {}

void Spell::cast() {

}

SpellType Spell::getType() {
    return type;
}

void initSpells() {
    
}