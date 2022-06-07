// 
// Copyright 2022 Clemens Cords
// Created on 6/5/22 by clem (mail@clemens-cords.com)
//

#pragma once

#include <include/physics/collision_shape.hpp>

namespace ts
{
    /// \brief collision shape: a 2-point line
    class CollisionLine : public CollisionShape
    {
        public:
            /// \brief construct from 2 points
            /// \param a: first point
            /// \param b: second point
            CollisionLine(Vector2f a, Vector2f b);

        protected:
            b2Shape* get_native_shape() override;

        private:
            b2EdgeShape _shape;
    };
}