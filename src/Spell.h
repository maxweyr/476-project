#ifndef SPELL_H
#define SPELL_H

#include <iostream>
#include <string>
#include <glad/glad.h>

using namespace std;

typedef enum {
    NO_SPELL,
    FIRE,
    ICE,
    LIGHTNING
} SpellType;

class Spell {
    private:
        std::string name;
        SpellType type;
        float damage;
        float fireRate;
        float range;
        float cooldown;
        SpellType weakness;

    public:
        Spell(std::string name, SpellType type, float damage, float fireRate, float range, float cooldown, SpellType weakness);
        void cast();
        SpellType getType();
};

void initSpells();

#endif // SPELL_H