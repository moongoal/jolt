#ifndef JLT_MEDIA_MODEL_HPP
#define JLT_MEDIA_MODEL_HPP

namespace jolt::media {
    class Model {
        public:
        Model();
        Model(Model const &other) = delete;
        Model(Model &&other);
    };
}

#endif /* JLT_MEDIA_MODEL_HPP */
