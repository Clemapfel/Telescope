// 
// Copyright 2022 Joshua Higginbotham
// Created on 6/9/22 by clem (mail@clemens-cords.com | https://github.com/Clemapfel)
//

#pragma once

#include <include/triangle_shape.hpp>
#include <include/physics/collision_polygon.hpp>

namespace ts
{
    //
    class CollisionTriangleShape : public TriangleShape, public CollisionPolygon
    {
        public:
            /// \brief virtual destructor
            virtual CollisionTriangleShape() = default;

            //
            CollisionTriangleShape(PhysicsWorld*, CollisionType, Vector2f a, Vector2f b, Vector2f c);

            //
            void update();
    };
}