# samegame-benchmark
Samegame fork containing benchmarks for the blog post [QML vs. C++ for application startup time](https://woboq.com/blog/qml-vs-cpp-for-application-startup-time.html)


# Build
- Patch your qtdeclarative with qtdeclarative-patches/0001-Privately-export-QQuickSpringAnimation.patch
- Build Qt
- Build this example: `qmake && make`

# Running
The benchmark will run first and the game will start afterward (just press the Menu button once)

