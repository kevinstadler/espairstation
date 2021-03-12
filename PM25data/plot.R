# INDOOR DATA
d <- read.csv('data.csv', col.names=c('t', 'ng7', 'g7pm1', 'g7pm25', 'g7pm10', 'npt', 'ptpm1', 'ptpm25', 'ptpm10'))
d$minute <- d$t / 60000

plot(g7pm25 ~ minute, d, type='l', lty=1)
lines(ptpm25 ~ minute, d, lty=2)
lines(g7pm10 ~ minute, d, col='gray', lty=1)
lines(ptpm10 ~ minute, d, col='gray', lty=2)
#lines(g7pm1 ~ minute, d, col='red', lty=1)
#lines(ptpm1 ~ minute, d, col='red', lty=2)
# start (minute 0) is ~13:50
# at minute 115 ellen turned fan from 3 down to 1 (at 15:45)
# finish (minute 200) is ~17:10
#plot(g7pm25 - ptpm25 ~ ptpm25, d)

# OUTDOOR DATA

d <- read.csv('outdoor.csv', col.names=c('t', 'ng7', 'g7pm1', 'g7pm25', 'g7pm10', 'npt', 'ptpm1', 'ptpm25', 'ptpm10'))
d$minute <- d$t / 60000 # last one was 11pm on 2021-02-18

# 11pm 10pm 9pm 8pm 7pm 6pm
aqi <- data.frame(minute=c(324, 264, 204, 144, 84, 24), pm25=c(53, 52, 36, 26, 21, 19), pm10=c(70, 79, 66, 58, 55, 35))

plot(g7pm25 ~ minute, d, type='l', lty=1, ylim=c(0,70))
lines(ptpm25 ~ minute, d, lty=2)
points(pm25 ~ minute, aqi, pty=21)


plot(g7pm10 ~ minute, d, type='l', lty=1, ylim=c(0,100))
lines(ptpm10 ~ minute, d, lty=2)
points(pm10 ~ minute, aqi, pty=21)

# ROOM CLEANING
# at 500.000 i briefly opened the door, at 600.000 turned the fan off?
d <- read.csv('room.csv', col.names=c('t', 'ng7', 'g7cf1pm1', 'g7cf1pm25', 'g7cf1pm10', 'g7pm1', 'g7pm25', 'g7pm10', 'npt', 'ptcf1pm1', 'ptcf1pm25', 'ptcf1pm10', 'ptpm1', 'ptpm25', 'ptpm10'))
d$minute <- d$t / 60000 # last one was 11pm on 2021-02-18

plot(g7cf1pm25 ~ minute, d, type='l', lty=1, ylim=c(0,100))
lines(ptcf1pm25 ~ minute, d, lty=2)
lines(g7pm25 ~ minute, d, lty=1, col='gray')
lines(ptpm25 ~ minute, d, lty=2, col='gray')

# OUTDOOR ALL MEASURES this is a good one

d <- read.csv('outdoorfull.csv', col.names=c('t', 'ng7', 'g7cf1pm1', 'g7cf1pm25', 'g7cf1pm10', 'g7pm1', 'g7pm25', 'g7pm10', 'npt', 'ptcf1pm1', 'ptcf1pm25', 'ptcf1pm10', 'ptpm1', 'ptpm25', 'ptpm10'))
d$minute <- d$t / 60000 # last one (498) was 5pm on 2021-02-19

# 5pm 4pm 3pm 2pm 1pm 12pm 11am 10am 9am?
aqi <- data.frame(minute=c(498, 438, 378, 318, 258, 198, 138, 78, 18), pm25=c(0, 14, 16, 20, 35, 44, 44, 51, 48), pm10=c(0, 5, 8, 16, 45, 42, 51, 69, 62))

plot(pm25 ~ minute, aqi, pty=21, xlim=c(0, 500), ylim=c(0, 120))
lines(g7cf1pm25 ~ minute, d, lty=1)
lines(ptcf1pm25 ~ minute, d, lty=2)
lines(g7pm25 ~ minute, d, lty=1, col='gray')
lines(ptpm25 ~ minute, d, lty=2, col='gray')
# conclusion: the Plantower atmospheric (non-CF=1) value tracks the official sensor closely

# correlation between the two
# for uncorrected values of the plantower, the g7 exaggerates with at least 10, up to 30 for higher values
plot(g7cf1pm25 ~ ptcf1pm25, d, xlim=c(0, 130), ylim=c(0, 130))
abline(a=0, b=1, lty=2)
# the g7 corrected tracks the plantower uncorrected!!!
points(g7pm25 ~ ptcf1pm25, d, col='gray')

# for (better) corrected values of the plantower, the g7 is 10ug higher from the start, uncorrected g7 becomes 60ug at higher values, 20ug with corrected
plot(g7cf1pm25 ~ ptpm25, d, xlim=c(0, 130), ylim=c(0, 130))
abline(a=0, b=1, lty=2)
points(g7pm25 ~ ptpm25, d, col='gray')


# difference between the two
#plot(g7cf1pm25 - ptcf1pm25 ~ minute, d, ylim=c(0, 40), main='g7 exaggeration')
#points(g7pm25 - ptpm25 ~ minute, d, col='gray')
plot(g7cf1pm25 - ptcf1pm25 ~ ptcf1pm25, d, xlim=c(0, 130), ylim=c(0, 30), main='weird g7 exaggeration because of scaling??')
points(g7pm25 - ptpm25 ~ ptcf1pm25, d, col='gray')
plot(g7cf1pm25 - ptcf1pm25 ~ ptpm25, d, xlim=c(0, 130), ylim=c(0, 30), main='weird g7 exaggeration because of scaling??')
points(g7pm25 - ptpm25 ~ ptpm25, d, col='gray')


# check out the transform curves -- WEIRD for the G7
d$g7fac <- d$g7pm25 / d$g7cf1pm25
d$ptfac <- d$ptpm25 / d$ptcf1pm25
plot(d$g7fac, type='l')
lines(d$ptfac, lty=2)

plot(g7fac ~ g7pm25, d, ylim=c(.6, 1.2))
points(ptfac ~ ptpm25, d, col='gray')



## USAGE CONCLUSION
# 1. best: use the plantower corrected
# 2. best: use the g7 corrected and deduct 10-12ug from values up to 50 (which is all we need anyway), deduct much more above 50...
