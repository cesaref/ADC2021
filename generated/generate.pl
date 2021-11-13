#!/usr/bin/perl

use strict;

sub execute ($)
{
    my ($command) = @_;

    print "> ${command}\n";
    if (system ("${command}") != 0)
    {
        die "command failed";
    }

}

sub analyse ($)
{
    my ($commandOptions) = @_;
    execute ("../tools/analyse/Builds/MacOSX/build/Debug/analyse ${commandOptions}");
}


analyse ("../samples/B3.wav --frequency=130.7 --harmonics=48 --template=../template/generate.soul >b3.generate.soul --threshhold=-110");
analyse ("../samples/B3.wav --frequency=130.7 --harmonics=48 --template=../template/instrument.soul >b3.instrument.soul --threshhold=-110");
analyse ("../samples/B3.wav --frequency=130.7 --harmonics=48 --template=../template/modifiers.soul >b3.modifiers.soul --threshhold=-110");
execute ("soul render b3.generate.soul --output=b3.rendered.wav --length=352800");

analyse ("../samples/Crash.wav --frequency=419.4 --harmonics=64 --template=../template/generate.soul >crash.generate.soul --threshhold=-110");
analyse ("../samples/Crash.wav --frequency=419.4 --harmonics=64 --template=../template/instrument.soul >crash.instrument.soul --threshhold=-110");
analyse ("../samples/Crash.wav --frequency=419.4 --harmonics=64 --template=../template/modifiers.soul >crash.modifiers.soul --threshhold=-110");
execute ("soul render crash.generate.soul --output=crash.rendered.wav --length=352800");

analyse ("../samples/Equator.wav --frequency=130.7 --harmonics=128 --template=../template/generate.soul >equator.generate.soul --threshhold=-110");
analyse ("../samples/Equator.wav --frequency=130.7 --harmonics=128 --template=../template/instrument.soul >equator.instrument.soul --threshhold=-110");
analyse ("../samples/Equator.wav --frequency=130.7 --harmonics=128 --template=../template/modifiers.soul >equator.modifiers.soul --threshhold=-110");
execute ("soul render equator.generate.soul --output=equator.rendered.wav --length=529200");

analyse ("../samples/Rhodes.wav --frequency=130.7 --harmonics=48 --template=../template/generate.soul >rhodes.generate.soul --threshhold=-95");
analyse ("../samples/Rhodes.wav --frequency=130.7 --harmonics=48 --template=../template/instrument.soul >rhodes.instrument.soul --threshhold=-95");
analyse ("../samples/Rhodes.wav --frequency=130.7 --harmonics=48 --template=../template/modifiers.soul >rhodes.modifiers.soul --threshhold=-95");
execute ("soul render rhodes.generate.soul --output=rhodes.rendered.wav --length=441000");
