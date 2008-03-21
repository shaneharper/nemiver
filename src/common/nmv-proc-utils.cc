// Author: Dodji Seketeli
/*
 *This file is part of the Nemiver project
 *
 *Nemiver is free software; you can redistribute
 *it and/or modify it under the terms of
 *the GNU General Public License as published by the
 *Free Software Foundation; either version 2,
 *or (at your option) any later version.
 *
 *Nemiver is distributed in the hope that it will
 *be useful, but WITHOUT ANY WARRANTY;
 *without even the implied warranty of
 *MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *See the GNU General Public License for more details.
 *
 *You should have received a copy of the
 *GNU General Public License along with Nemiver;
 *see the file COPYING.
 *If not, write to the Free Software Foundation,
 *Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *See COPYRIGHT file copyright information.
 */
#include <cstring>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <pty.h>
#include <termios.h>
#include <vector>
#include <memory>
#include "nmv-proc-utils.h"
#include "nmv-exception.h"
#include "nmv-log-stream-utils.h"

namespace nemiver {
namespace common {

bool
launch_program (const std::vector<UString> &a_args,
                int &a_pid,
                int &a_master_pty_fd,
                int &a_stdout_fd,
                int &a_stderr_fd)
{
    RETURN_VAL_IF_FAIL (!a_args.empty (), false) ;

    //logging stuff
    UString str ;
    for (std::vector<UString>::const_iterator it=a_args.begin ();
         it != a_args.end ();
         ++it) {
        str += *it + " " ;
    }
    LOG_D ("launching program with args: '" << str << "'", NMV_DEFAULT_DOMAIN) ;

    enum ReadWritePipe {
        READ_PIPE=0,
        WRITE_PIPE=1
    };

    int stdout_pipes[2] = {0};
    int stderr_pipes[2]= {0} ;
    //int stdin_pipes[2]= {0} ;
    int master_pty_fd (0) ;

    RETURN_VAL_IF_FAIL (pipe (stdout_pipes) == 0, false) ;
    //argh, this line leaks the preceding pipes if it fails
    RETURN_VAL_IF_FAIL (pipe (stderr_pipes) == 0, false) ;
    //RETURN_VAL_IF_FAIL (pipe (stdin_pipes) == 0, false) ;

    char pts_name[256]={0} ;
    int pid  = forkpty (&master_pty_fd, pts_name, NULL, NULL);
    LOG_DD ("process forked. pts_name: '"
            << pts_name << "', pid: '" << pid << "'") ;

    //int pid  = fork () ;
    if (pid == 0) {
        //in the child process
        //*******************************************
        //wire stderr to stderr_pipes[WRITE_PIPE] pipe
        //******************************************
        close (2) ;
        dup (stderr_pipes[WRITE_PIPE]) ;

        //*******************************************
        //wire stdout to stdout_pipes[WRITE_PIPE] pipe
        //******************************************
        close (1) ;
        dup (stdout_pipes[WRITE_PIPE]) ;

        //*****************************
        //close the unnecessary pipes
        //****************************
        close (stderr_pipes[READ_PIPE]) ;
        close (stdout_pipes[READ_PIPE]) ;
        //close (stdin_pipes[WRITE_PIPE]) ;

        //**********************************************
        //configure the pipes to be to have no buffering
        //*********************************************
        int state_flag (0) ;
        if ((state_flag = fcntl (stdout_pipes[WRITE_PIPE],
                        F_GETFL)) != -1) {
            fcntl (stdout_pipes[WRITE_PIPE],
                    F_SETFL,
                    O_SYNC | state_flag) ;
        }
        if ((state_flag = fcntl (stderr_pipes[WRITE_PIPE],
                        F_GETFL)) != -1) {
            fcntl (stderr_pipes[WRITE_PIPE],
                    F_SETFL,
                    O_SYNC | state_flag) ;
        }

        std::auto_ptr<char *> args;
        args.reset (new char* [a_args.size () + 1]) ;
        memset (args.get (), 0,
                sizeof (char*) * (a_args.size () + 1)) ;
        if (!args.get ()) {
            exit (-1) ;
        }
        std::vector<UString>::const_iterator iter ;
        unsigned int i (0) ;
        for (i=0 ; i < a_args.size () ; ++i) {
            args.get ()[i] =
            const_cast<char*> (a_args[i].c_str ());
        }

        execvp (args.get ()[0], args.get ()) ;
        exit (-1) ;
    } else if (pid > 0) {
        //in the parent process

        //**************************
        //close the useless pipes
        //*************************
        close (stderr_pipes[WRITE_PIPE]) ;
        close (stdout_pipes[WRITE_PIPE]) ;
        //close (stdin_pipes[READ_PIPE]) ;

        //****************************************
        //configure the pipes to be non blocking
        //****************************************
        int state_flag (0) ;
        if ((state_flag = fcntl (stdout_pipes[READ_PIPE], F_GETFL)) != -1) {
            fcntl (stdout_pipes[READ_PIPE], F_SETFL, O_NONBLOCK|state_flag);
        }
        if ((state_flag = fcntl (stderr_pipes[READ_PIPE], F_GETFL)) != -1) {
            fcntl (stderr_pipes[READ_PIPE], F_SETFL, O_NONBLOCK|state_flag);
        }

        if ((state_flag = fcntl (master_pty_fd, F_GETFL)) != -1) {
            fcntl (master_pty_fd, F_SETFL, O_NONBLOCK|state_flag);
        }
        struct termios termios_flags;
        tcgetattr (master_pty_fd, &termios_flags);
        termios_flags.c_iflag &= ~(IGNPAR | INPCK
                |INLCR | IGNCR
                | ICRNL | IXON
                |IXOFF | ISTRIP);
        termios_flags.c_iflag |= IGNBRK | BRKINT | IMAXBEL | IXANY;
        termios_flags.c_oflag &= ~OPOST;
        termios_flags.c_cflag &= ~(CSTOPB | CREAD | PARENB | HUPCL);
        termios_flags.c_cflag |= CS8 | CLOCAL;
        termios_flags.c_cc[VMIN] = 0;
        //echo off
        termios_flags.c_lflag &= ~(ECHOKE | ECHOE |ECHO| ECHONL | ECHOPRT
                |ECHOCTL | ISIG | ICANON
                |IEXTEN | NOFLSH | TOSTOP);
        cfsetospeed(&termios_flags, __MAX_BAUD);
        tcsetattr(master_pty_fd, TCSANOW, &termios_flags);
        a_pid = pid ;
        a_master_pty_fd = master_pty_fd ;
        a_stdout_fd = stdout_pipes[READ_PIPE] ;
        a_stderr_fd = stderr_pipes[READ_PIPE] ;
    } else {
        //the fork failed.
        close (stderr_pipes[READ_PIPE]) ;
        close (stdout_pipes[READ_PIPE]) ;
        LOG_ERROR ("fork() failed\n") ;
        return false ;
    }
    return true ;
}

void
attach_channel_to_loop_context_as_source
                        (Glib::IOCondition a_cond,
                         const sigc::slot<bool, Glib::IOCondition> &a_slot,
                         const Glib::RefPtr<Glib::IOChannel> &a_chan,
                         const Glib::RefPtr<Glib::MainContext>&a_ctxt)
{
    THROW_IF_FAIL (a_chan) ;
    THROW_IF_FAIL (a_ctxt) ;

    Glib::RefPtr<Glib::IOSource> io_source =
                                    Glib::IOSource::create (a_chan, a_cond) ;
    io_source->connect (a_slot) ;
    io_source->attach (a_ctxt) ;
}
}//end namespace
}//end namespace nemiver

