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

## Part 2A. Minimal Custom HLSL Alternative

If the normal material graph feels too noisy, use one `Custom` node and keep the rest of the graph minimal.

This version uses `CustomDepth` edge detection instead of a full multi-node stencil graph.

That means:

- your C++ code still decides which actor writes to custom depth
- the material only detects silhouette edges around those marked pixels
- the graph stays very small

## Minimal graph layout

Create only these nodes:

1. `ScreenPosition`
2. `Custom` node
3. `ScalarParameter` named `OutlineThicknessPx`
4. `VectorParameter` named `OutlineColor`
5. `SceneTexture` node set to `CustomDepth`
6. `SceneTexture` node set to `PostProcessInput0`

Connect them like this:

1. `ScreenPosition` -> use the viewport UV output into the `Custom` input named `UV`
2. `OutlineThicknessPx` -> `Custom` input named `ThicknessPx`
3. `OutlineColor` -> `Custom` input named `OutlineColor`
4. `SceneTexture(CustomDepth).Color` -> `Custom` input named `DummyCustomDepth`
5. `SceneTexture(PostProcessInput0).Color` -> `Custom` input named `DummySceneColor`
6. `Custom` output -> `Emissive Color`

Set the `Custom` node output type to `CMOT Float3`.

## Custom node inputs

Add these inputs to the `Custom` node:

1. `UV` as `float2`
2. `ThicknessPx` as `float`
3. `OutlineColor` as `float3`
4. `DummyCustomDepth` as `float4`
5. `DummySceneColor` as `float4`

## Custom node code

Paste this into the `Custom` node:

```hlsl
float2 texel = View.ViewSizeAndInvSize.zw * max(1.0, ThicknessPx);

// These dummy inputs force the material translator to include scene texture plumbing for this custom node.
// Without them some UE versions fail with: use of undeclared identifier 'SceneTextureLookup'.
float translatorKeepAlive = DummyCustomDepth.r * 0.0 + DummySceneColor.r * 0.0;

float centerDepth = SceneTextureLookup(UV, 13, false).r;
float leftDepth = SceneTextureLookup(UV + float2(-texel.x, 0.0), 13, false).r;
float rightDepth = SceneTextureLookup(UV + float2(texel.x, 0.0), 13, false).r;
float upDepth = SceneTextureLookup(UV + float2(0.0, -texel.y), 13, false).r;
float downDepth = SceneTextureLookup(UV + float2(0.0, texel.y), 13, false).r;

// Convert raw depth into a simple binary occupancy mask first.
// Comparing raw custom-depth values directly tends to light up the whole character because depth changes
// naturally across the mesh surface, not only on the silhouette.
float centerMask = step(0.00001, centerDepth);
float leftMask = step(0.00001, leftDepth);
float rightMask = step(0.00001, rightDepth);
float upMask = step(0.00001, upDepth);
float downMask = step(0.00001, downDepth);

float edgeMask = 0.0;
edgeMask = max(edgeMask, abs(centerMask - leftMask));
edgeMask = max(edgeMask, abs(centerMask - rightMask));
edgeMask = max(edgeMask, abs(centerMask - upMask));
edgeMask = max(edgeMask, abs(centerMask - downMask));

float3 sceneColor = SceneTextureLookup(UV, 14, false).rgb;
sceneColor += translatorKeepAlive;
return lerp(sceneColor, OutlineColor, saturate(edgeMask));
```

## What those texture IDs mean

In this snippet:

- `13` = `CustomDepth`
- `14` = `PostProcessInput0`

That is the standard mapping used by Unreal material scene-texture lookup in post-process materials.

## Recommended first parameter values

Start with:

1. `OutlineThicknessPx = 1.0`
2. `OutlineColor = (1.0, 0.65, 0.1)`

If the outline is too thin:

1. raise `OutlineThicknessPx` to `1.5`
2. then to `2.0` if needed

## Important caveats

### 1. This version keys off `CustomDepth`, not stencil categories

It is ideal for a first minimal setup, but later you may still want a stencil-aware version if you need:

- different colors for ally/enemy/active target
- different outline styles per category

### 2. Some engine versions need SceneTexture nodes wired into the Custom node

If the material compiler complains that `SceneTextureLookup` is unknown:

1. add a `SceneTexture` node set to `CustomDepth`
2. add another `SceneTexture` node set to `PostProcessInput0`
3. connect both of their `Color` outputs into dummy `float4` inputs on the `Custom` node
4. keep the `translatorKeepAlive` line in the HLSL code
5. compile again

This is a common Unreal material translator quirk when using scene texture functions inside a `Custom` node.

## Minimal stencil-aware follow-up

Once this first version works, the next upgrade is to switch the edge detection from `CustomDepth` to `CustomStencil` and branch colors by stencil value.

That makes the graph slightly more complex, but still far smaller than a fully hand-built node network.

## Part 2B. Recommended Minimal CustomStencil Version

For this project, this is the recommended version.

Why it is better than the `CustomDepth` variant:

- it produces a cleaner binary mask
- it is much less likely to fill the whole character
- it is better aligned with future categories such as hovered enemy, active unit, ally, or interactable

The graph is almost the same as the previous `Custom` node version.

## Minimal graph layout for stencil

Create these nodes:

1. `ScreenPosition`
2. `Custom` node
3. `ScalarParameter` named `OutlineThicknessPx`
4. `VectorParameter` named `OutlineColor`
5. `SceneTexture` node set to `CustomStencil`
6. `SceneTexture` node set to `PostProcessInput0`

Connect them like this:

1. `ScreenPosition` -> viewport UV into the `Custom` input named `UV`
2. `OutlineThicknessPx` -> `Custom` input named `ThicknessPx`
3. `OutlineColor` -> `Custom` input named `OutlineColor`
4. `SceneTexture(CustomStencil).Color` -> `Custom` input named `DummyCustomStencil`
5. `SceneTexture(PostProcessInput0).Color` -> `Custom` input named `DummySceneColor`
6. `Custom` output -> `Emissive Color`

Set the `Custom` node output type to `CMOT Float3`.

## Custom node inputs for stencil

Add these inputs to the `Custom` node:

1. `UV` as `float2`
2. `ThicknessPx` as `float`
3. `OutlineColor` as `float3`
4. `DummyCustomStencil` as `float4`
5. `DummySceneColor` as `float4`

## CustomStencil HLSL code

Paste this into the `Custom` node:

```hlsl
float2 texel = View.ViewSizeAndInvSize.zw * max(1.0, ThicknessPx);

// Dummy inputs keep scene texture plumbing alive for the material translator.
float translatorKeepAlive = DummyCustomStencil.r * 0.0 + DummySceneColor.r * 0.0;

float centerStencil = SceneTextureLookup(UV, 25, false).r;
float leftStencil = SceneTextureLookup(UV + float2(-texel.x, 0.0), 25, false).r;
float rightStencil = SceneTextureLookup(UV + float2(texel.x, 0.0), 25, false).r;
float upStencil = SceneTextureLookup(UV + float2(0.0, -texel.y), 25, false).r;
float downStencil = SceneTextureLookup(UV + float2(0.0, texel.y), 25, false).r;

// Treat any non-zero stencil as highlighted for the current prototype.
float centerMask = step(0.00001, centerStencil);
float leftMask = step(0.00001, leftStencil);
float rightMask = step(0.00001, rightStencil);
float upMask = step(0.00001, upStencil);
float downMask = step(0.00001, downStencil);

float edgeMask = 0.0;
edgeMask = max(edgeMask, abs(centerMask - leftMask));
edgeMask = max(edgeMask, abs(centerMask - rightMask));
edgeMask = max(edgeMask, abs(centerMask - upMask));
edgeMask = max(edgeMask, abs(centerMask - downMask));

float3 sceneColor = DummySceneColor.rgb;
sceneColor += translatorKeepAlive;
return lerp(sceneColor, OutlineColor, saturate(edgeMask));
```

## Texture ID used here

In Unreal Engine 5.7:

- `25` = `PPI_CustomStencil`

That comes directly from `MaterialSceneTextureId.h` in the engine source.

## Why this version should outline only the contour

This version does not compare raw depth changes.

Instead it compares only a binary stencil mask:

- `0` = pixel is not part of a highlighted object
- `1` = pixel is part of a highlighted object

So only pixels where the mask changes between center and neighbor become outline pixels.

## Important runtime expectation

This version assumes:

1. the actor has `Render CustomDepth` enabled when hovered
2. the actor writes a non-zero custom stencil value

Your current code path already does this when highlight is enabled.

## If you later want multiple highlight colors

Right now the code uses a single stencil value, so `stencil > 0` is enough.

Later you can branch by exact stencil category instead of only using non-zero mask.

Example future pattern:

1. `1` = hovered enemy
2. `2` = active unit
3. `3` = ally
4. `4` = interactable

## Additionaly

To prevent flickering of outline go to material, click on biggest vertical block (named same as the filename) and change Blendable Location to "Scene Color Before DOF"
(hope it won't affect anything else in bad manner)

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

That usually means the shader is comparing raw `CustomDepth` values instead of a binary mask.

Use this rule:

1. convert each depth sample to `0` or `1` with `step`
2. compare the center mask with neighbor masks
3. build the outline only from those mask differences

If you compare raw depth values directly, the whole character can light up because depth changes across the body even when you are not on the outer contour.

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
