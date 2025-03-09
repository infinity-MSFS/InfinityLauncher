#pragma once

namespace Infinity {
    class Layer {
    public:
        virtual ~Layer() = default;

        virtual void OnAttach() = 0;

        virtual void OnDetach() = 0;

        virtual void OnUpdate(float ts) = 0;

        virtual void OnUIRender() = 0;
    };
} // namespace Infinity
