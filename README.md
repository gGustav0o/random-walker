# random-walker

**random-walker** is a desktop image segmentation application built with C++,
Qt 6, Eigen, spdlog, and CMake.

The application implements an interactive Random Walker segmentation workflow:
the user marks a few background and object regions, the image is represented as
a weighted pixel graph, and labels are propagated from the marked pixels to the
unknown pixels. The result is shown as an overlay on the source image.

## Usage

1. Start the application.
2. Click **Open** and select an image.
3. Choose a label: **Background** or **Object**.
4. Draw one or more seed rectangles for both labels.
5. Optionally click **Auto markers** to generate and inspect proposed markers.
6. Click **Run segmentation**.
7. Use **Clear seeds** or **Clear image** to reset the current work.

Random Walker requires at least one background marker and one object marker.
Automatic markers are only proposed constraints; they do not remove the need for
a well-posed segmentation setup.

## Input Images

- Supported formats depend on Qt image loading support; common formats such as
  PNG, JPEG, and BMP are supported.
- Loaded images are converted to grayscale before segmentation.
- The current segmentation limit is `1,048,576` pixels.
- Clear contrast between the marked regions usually produces better results.

## Parameters

### Graph

These parameters control how labels propagate through the image.

- `Connectivity`: chooses which neighboring pixels are connected in the graph.
  Use `4-connectivity` for stricter propagation through horizontal and vertical
  neighbors. Use `8-connectivity` when diagonal continuity matters or when the
  result looks too blocky.
- `Distance p`: controls how strongly diagonal edges are weakened in
  `8-connectivity`. `0` treats diagonal and orthogonal edges equally. Larger
  values make diagonal propagation more conservative. This parameter has no
  visible effect for purely orthogonal `4-connectivity`.
- `Beta`: controls sensitivity to intensity changes. Lower values let labels
  cross weak intensity changes more easily, producing smoother but leakier
  results. Higher values make the segmentation follow contrast more tightly,
  but very high values can prevent labels from crossing noisy or low-contrast
  areas.

Practical tuning:

| Symptom | Try |
| ------- | --- |
| Object label leaks into the background | Increase `Beta`; add more background seeds near the leak |
| Object region is fragmented or does not grow enough | Decrease `Beta`; add object seeds inside missing areas |
| Diagonal structures look broken | Use `8-connectivity`; reduce `Distance p` |
| Diagonal propagation is too permissive | Increase `Distance p` |

### Auto Markers


- `Object polarity`: tells the marker proposal whether the object is expected
  to be darker or brighter than the background. Choose the option that matches
  the target region in the loaded grayscale image.
- `Confidence`: minimum certainty required for an automatic marker candidate.
  Higher values create fewer and safer markers. Lower values create more
  markers, but can include unreliable pixels when object and background
  intensities overlap.
- `Safe margin, px`: removes candidate pixels too close to the boundary of a
  proposed region. Increase it when automatic markers touch uncertain edges.
  Set it to `0` to disable this cleanup.
- `Min area, px`: removes connected marker components smaller than the chosen
  area. Increase it to suppress isolated noise. Decrease it when valid target
  regions are small.

## Documentation

- [Build instructions](BUILD.md)
- [Coding style](CODESTYLE.md)
- [Random Walker algorithm notes](docs/random-walker-algorithm.md)

## License

BSD 3-Clause License. See [LICENSE](LICENSE).
