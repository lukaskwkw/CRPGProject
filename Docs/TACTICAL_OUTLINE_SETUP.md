# Tactical Outline Setup

This guide walks through the editor-side setup for the hover outline used by tactical combat targeting.

Code support is already in place:

- hovered enemy targets enable `Render CustomDepth`
- hovered targets write a custom stencil value from `ACRPGBaseCharacter`
- `r.CustomDepth=3` is already enabled in project config

What is still missing is the Unreal asset setup that turns that stencil/custom-depth data into a visible outline.

## Goal

After this setup:

- hovering an enemy in combat targeting mode should visibly outline that unit in the world
- the effect should come from a post-process material driven by `CustomStencil`

## Recommended Setup

Use `CustomStencil`, not only `CustomDepth`.

Why:

- `CustomDepth` tells you that an object was marked
- `CustomStencil` lets you distinguish target types later
- it gives you room for future values such as ally, enemy, active unit, interactable, or danger marker

## Part 1. Create the material

### 1. Create a new material asset

In Content Browser:

1. Right click in the folder where you keep rendering/UI helper assets.
2. Choose `Material`.
3. Name it something like `M_PP_TacticalOutline`.

### 2. Open the material and change core settings

In the material details panel:

1. Set `Material Domain` to `Post Process`.
2. Leave it unlit.
3. Make sure the material outputs into `Emissive Color`.

You do not need a surface material for this effect.

## Part 2. Build the basic outline logic

The simplest version is:

- read current pixel stencil or custom depth
- sample neighboring pixels
- if neighbors differ, treat this pixel as an edge
- blend an outline color over the existing scene color

### 3. Add the scene color input

1. Add a `SceneTexture` node.
2. Set `Scene Texture Id` to `PostProcessInput0`.

This is your current rendered scene.

### 4. Add the stencil input

1. Add another `SceneTexture` node.
2. Set `Scene Texture Id` to `CustomStencil`.

This is the stencil mask written by hovered actors.

### 5. Sample neighboring pixels

You need to compare the current stencil sample against nearby UV offsets.

Create these nodes:

1. `ScreenPosition`
2. `ComponentMask` for RG if needed
3. `ViewSize` or `SceneTexelSize` style node depending on the editor version and available nodes
4. four offset samples:
   - left
   - right
   - up
   - down

Use offsets of one texel in screen space.

Conceptually:

- current stencil at UV
- stencil at UV + left
- stencil at UV + right
- stencil at UV + up
- stencil at UV + down

### 6. Detect the edge

Treat a pixel as outline when:

- current pixel belongs to marked object, or a neighbor belongs to marked object,
- and at least one compared sample differs from the current sample.

In practice, the easy material-graph version is:

1. compare each neighbor to current stencil using `NotEqual` or equivalent subtract/abs/greater-than setup
2. combine neighbor differences with `Max`
3. multiply by a mask that checks whether the current or adjacent stencil is greater than `0`

This gives you a binary-ish edge mask.

### 7. Pick outline color

Add a `Vector3` or `Constant3Vector` node.

Recommended first color:

- warm tactical orange or yellow
- something clearly readable against terrain and characters

Example starting point:

- RGB around `1.0, 0.65, 0.1`

### 8. Blend outline over the scene

Use:

1. `Lerp`
2. `A = PostProcessInput0`
3. `B = SceneColor + OutlineColor` or simply the outline color stylized as you prefer
4. `Alpha = EdgeMask`

Then connect the result to `Emissive Color`.

For a first pass, a plain outline color is enough.

## Part 3. Make it respect stencil values

Right now the code writes a stencil value for hovered combat targets.

If you want to support different colors later:

1. compare the `CustomStencil` sample against concrete values
2. use those comparisons to choose different colors

Example future mapping:

- `1` = hovered enemy
- `2` = active unit
- `3` = ally highlight
- `4` = interactable object

For now, if your code only writes one value, it is enough to check `CustomStencil > 0`.

## Part 4. Apply the material in the level

### 9. Add or select a Post Process Volume

In the level:

1. Place a `Post Process Volume` if you do not already have one.
2. Enable `Infinite Extent (Unbound)` if you want the effect active everywhere.

### 10. Add the material as a blendable

Inside the Post Process Volume:

1. Find `Rendering Features` or the post-process material section.
2. Add an entry to `Blendables`.
3. Assign `M_PP_TacticalOutline`.

## Part 5. Verify the setup

### 11. Test in tactical combat targeting mode

Check this exact sequence:

1. enter tactical turn mode
2. press melee or ranged attack
3. hover an enemy unit

Expected result:

- the enemy mesh should now be outlined by the post-process material

If there is still no visible outline, the code path may still be fine and only the material graph may be wrong.

## Troubleshooting

### Outline still does not appear

Check these in order:

1. confirm the Post Process Volume is active and unbound
2. confirm the material domain is really `Post Process`
3. confirm the material is assigned in `Blendables`
4. confirm the hovered actor writes `CustomDepth`
5. confirm your graph checks `CustomStencil` or `CustomDepth` correctly
6. use the viewport buffer visualizers for `CustomDepth` and `CustomStencil`

If the actor is visible in `CustomDepth` or `CustomStencil` buffer view, the C++ side is working and only the material graph needs adjustment.

### The whole mesh glows instead of only edges

That means your mask is probably testing `stencil > 0` but not doing neighbor edge comparison correctly.

### The outline is too thin or too thick

Adjust the screen-space offset size:

- smaller offsets = thinner outline
- larger offsets = thicker outline

### The color is hard to read

Pick a higher-contrast outline color and consider darkening the scene contribution behind the outline edge.

## Suggested asset naming

If you want to keep the content tree organized:

- `Content/UI/Materials/M_PP_TacticalOutline`
- `Content/UI/Materials/MI_PP_TacticalOutline_Enemy`

## Suggested next step after first success

Once the first outline works, the next useful improvement is:

1. move color/thickness into a material instance
2. reserve multiple stencil values for future highlight categories
3. make active unit and hovered enemy use distinct colors
