// VXScene - Vextryn Air Retained Scene Graph
// Provides a retained-mode node graph for building complex UIs with
// layout, clipping, z-ordering, and rendering passes.
#ifndef VXSCENE_HPP
#define VXSCENE_HPP

#include "vxrender.hpp"
#include <stddef.h>

enum VxNodeType {
    VX_NODE_CONTAINER,
    VX_NODE_RECT,
    VX_NODE_ROUNDED_RECT,
    VX_NODE_TEXT,
    VX_NODE_IMAGE_BLIT,
    VX_NODE_SHADOW,
    VX_NODE_GRADIENT
};

struct VxNode {
    VxNodeType type;
    int x, y, w, h;
    bool visible;
    bool clip_children;
    int z_index;
    float opacity;
    
    // Tree structure
    VxNode* parent;
    VxNode* first_child;
    VxNode* next_sibling;

    VxNode(VxNodeType t) : type(t), x(0), y(0), w(0), h(0), visible(true), clip_children(false), z_index(0), opacity(1.0f), parent(nullptr), first_child(nullptr), next_sibling(nullptr) {}
    virtual ~VxNode() {}

    void add_child(VxNode* child) {
        child->parent = this;
        if (!first_child) {
            first_child = child;
        } else {
            // Insert sorted by z_index
            VxNode* prev = nullptr;
            VxNode* curr = first_child;
            while (curr && curr->z_index <= child->z_index) {
                prev = curr;
                curr = curr->next_sibling;
            }
            if (!prev) {
                child->next_sibling = first_child;
                first_child = child;
            } else {
                child->next_sibling = prev->next_sibling;
                prev->next_sibling = child;
            }
        }
    }

    virtual void render_self(int abs_x, int abs_y) = 0;

    void render(int parent_x, int parent_y) {
        if (!visible || opacity <= 0.0f) return;
        int abs_x = parent_x + x;
        int abs_y = parent_y + y;

        // Push clip if needed
        VxClipGuard* guard = nullptr;
        if (clip_children) {
            guard = new VxClipGuard(abs_x, abs_y, w, h);
        }

        render_self(abs_x, abs_y);

        VxNode* child = first_child;
        while (child) {
            child->render(abs_x, abs_y);
            child = child->next_sibling;
        }

        if (guard) {
            delete guard;
        }
    }
};

struct VxRectNode : public VxNode {
    uint32_t color;
    VxRectNode() : VxNode(VX_NODE_RECT), color(0) {}
    void render_self(int abs_x, int abs_y) override {
        vxr_fill_rect(abs_x, abs_y, w, h, color);
    }
};

struct VxRoundedRectNode : public VxNode {
    uint32_t color;
    int radius;
    VxRoundedRectNode() : VxNode(VX_NODE_ROUNDED_RECT), color(0), radius(0) {}
    void render_self(int abs_x, int abs_y) override {
        vxr_rounded_rect(abs_x, abs_y, w, h, radius, color);
    }
};

// ... More nodes can be implemented easily ...

class VxScene {
public:
    VxNode* root;
    VxScene() : root(new VxRectNode()) {} // Root is a transparent container
    
    void render() {
        if (root) {
            root->render(0, 0);
        }
    }
};

#endif // VXSCENE_HPP
