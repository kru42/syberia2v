FROM vitasdk/vitasdk-softfp

# Clone the vitaGL repository
RUN git clone https://github.com/Rinnegatamante/vitaGL.git /opt/vitaGL

# Set working directory
WORKDIR /opt/vitaGL

# Build and install vitaGL with softfp ABI
RUN make SOFTFP_ABI=1 NO_DEBUG=1 SHARED_RENDERTARGETS=1 HAVE_GLSL_SUPPORT=1
RUN echo "vitaGL installed."

# Cleanup
RUN rm -rf /opt/vitaGL
