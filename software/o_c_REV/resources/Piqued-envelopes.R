# x = ((numpy.arange(0, 257) / 128.0 - 1.0))
x <- 0:256 / 128.0 - 1

x <- 0:256 / 128.0 - 1.0
x[length(x)] <- x[length(x) - 1]
violent_overdrive <- tanh(8.0 * x)
overdrive <- tanh(5.0 * x)
moderate_overdrive <- tanh(2.0 * x)

# Wavefolder curves from the first version
# tri_fold = numpy.abs(4.0 * x - numpy.round(4.0 * x)) * numpy.sign(x)
# sine_fold = numpy.sin(5 * numpy.pi * x)

tri_fold <- sin(pi * (3 * x + (2 * x) ** 3))

# In v1.4 RC
window <- exp(-x * x * 4) ** 1.5
sine <- sin(8 * pi * x)
sine_fold <- sine * window + atan(3 * x) * (1 - window)
sine_fold <- sine_fold / max(abs(sine_fold))

frequency = 4 / (1 + (1.5 * x) ** 2) ** 2
window2 = exp(-24 * max(abs(x) - 0.25, 0) ** 2)
sine2 = sin(2 * pi * frequency * x) * window2
cubic = (x ** 3 + 0.5 * x ** 2 + 0.5 * x) / 3
knee = min(x + 0.7, 0.0)
sine_fold2 = sine2 + cubic + knee

bdip <- atan(3 * x) * (1 - 3*window)
mdip <- atan(3 * x) * (1 - 1.4*window)
ldip <- atan(3 * x) * (1 - window)

env_quartic = x ** 3.32
env_expo = 1.0 - exp(-4 * x)

# lookup_tables.append(('env_linear', env_linear / env_linear.max() * 65535.0))
# lookup_tables.append(('env_expo', env_expo / env_expo.max() * 65535.0))
# lookup_tables.append(('env_quartic', env_quartic / env_quartic.max() * 65535.0))

raised_cosine = 0.5 - cos(x * pi) / 2

#plot(violent_overdrive)
#plot(overdrive)
#plot(moderate_overdrive)
#plot(tri_fold)
#plot(sine)
#plot(sine_fold)
#plot(sine_fold2)
#plot(env_quartic)
#plot(env_expo)
#plot(raised_cosine)

#sine_fold

env_sf <- as.integer(((sine_fold / max(sine_fold) * 65535) + 65535) / 2)

#plot(env_sf)
#cat(env_sf)
#length(env_sf)
env_sine <- as.integer(((moderate_overdrive / max(moderate_overdrive) * 65535) + 65535) / 2)

#plot(env_sine)
#fivenum(env_sine)
#cat(env_sine)
#length(env_sine)

#plot(env_sf)
#fivenum(env_sf)
#length(env_sf)

env_sqre <- as.integer(((violent_overdrive / max(violent_overdrive) * 65535) + 65535) / 2)
#plot(env_sqre)
#fivenum(env_sqre)
# cat(env_sqre)
#length(env_sqre)

env_bdip <- as.integer(((bdip / max(bdip) * 65535) + 65535) / 2)
plot(env_bdip)
fivenum(env_bdip)
cat(env_bdip)
length(env_bdip)

env_mdip <- as.integer(((mdip / max(mdip) * 65535) + 65535) / 2)
plot(env_mdip)
fivenum(env_mdip)
cat(env_mdip)
length(env_mdip)

env_ldip <- as.integer(((ldip / max(ldip) * 65535) + 65535) / 2)
plot(env_ldip)
fivenum(env_ldip)
cat(env_ldip)
length(env_ldip)

format_code <- function(env) {
  for (i in 1:257) {
    cat(paste(env[i],", ",sep=""))
    if (i %% 4 == 0) cat('\n')
  }
}

format_code(env_bdip)
format_code(env_mdip)
format_code(env_ldip)
