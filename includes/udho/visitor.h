/*
 * Copyright (c) 2018, Sunanda Bose (Neel Basu) (neel.basu.z@gmail.com) 
 * All rights reserved. 
 * 
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are met: 
 * 
 *  * Redistributions of source code must retain the above copyright notice, 
 *    this list of conditions and the following disclaimer. 
 *  * Redistributions in binary form must reproduce the above copyright 
 *    notice, this list of conditions and the following disclaimer in the 
 *    documentation and/or other materials provided with the distribution. 
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND ANY 
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY 
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT 
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY 
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH 
 * DAMAGE. 
 */

#ifndef UDHO_VISITOR_H
#define UDHO_VISITOR_H

#include <string>
#include <cstdint>
#include <algorithm>

namespace udho{    
    template <typename U>
    struct is_application{
        template <typename V>
        static typename V::application_type test(int);
        template <typename>
        static void test(...);
        enum {value = !std::is_void<decltype(test<U>(0))>::value};
    };
      
    template <typename VisitorT, typename ModuleT, bool IsApplication=is_application<ModuleT>::value>
    struct module_evaluator;
    
    template <typename VisitorT, typename ModuleT>
    struct module_evaluator<VisitorT, ModuleT, false>{
        // bare module
        VisitorT& _visitor;
        
        module_evaluator(VisitorT& visitor): _visitor(visitor){}
        void operator()(ModuleT& modul){
            _visitor._module(modul);
        }
    };
    
    
    template <typename VisitorT, typename ModuleT>
    struct module_evaluator<VisitorT, ModuleT, true>{
        // application
        VisitorT& _visitor;
        
        module_evaluator(VisitorT& visitor): _visitor(visitor){}        
        void operator()(ModuleT& app){
            _visitor._application(app);
        }
    };
    
    template <typename ActorT, typename ModuleT>
    void visit(ActorT& actor, ModuleT& mod){
        module_evaluator<ActorT, ModuleT> ev(actor);
        ev(mod);
    }
    template <typename VisitorT>
    struct visitor: VisitorT{
        using VisitorT::VisitorT;
        
        template <typename ModuleT>
        void operator()(ModuleT& overload){
            visit(*this, overload);
        }
        void operator()(){
            VisitorT::operator()();
        }
    };
        
    namespace visitors{
        struct visitable{
            static const std::uint8_t callable    = (1 << 0);
            static const std::uint8_t application = (1 << 1);
            static const std::uint8_t both        = callable | application;
        };
        
        template <std::uint8_t Visitables, typename StreamT>
        struct printing_visitor{
            StreamT& _stream;
            std::size_t _indent;
            
            printing_visitor(StreamT& stream): _stream(stream), _indent(0){}
            template <typename V>
            StreamT& column(const V& v, int width){
                _stream << std::left << std::setw(width) << std::setfill(' ') << v;
                return _stream;
            }
            template <typename ModuleT>
            void _module(const ModuleT& overload){
                if(Visitables & visitable::callable){
                    auto info = overload.info();
                    std::string indentation;
                    std::fill_n(std::back_inserter(indentation), _indent, '\t');
                    _stream << indentation;
                    column("module", 12);
                    column(info._method, 5);
                    column(info._pattern, 35);
                    column(" -> ", 4);
                    column(info._fptr, 16);
                    column(info._compositor, 25);
                    _stream << std::endl;
                }
            }
            template <typename AppT>
            void _application(const AppT& app){
                if(Visitables & visitable::application){
                    _indent++;
                    column("application", 12);
                    column(app.name(), 16);
                    column(" -> ", 4);
                    column(app._path, 16);
                    column(&app, 16);
                    _stream << std::endl;
                }
            }
            void operator()(){
                _indent--;
            }
        };
        
        template <std::uint8_t Visitables, typename StreamT>
        struct printing_visitor_html{
            StreamT& _stream;
            std::size_t _indent;
            
            printing_visitor_html(StreamT& stream): _stream(stream), _indent(0){}
            template <typename ModuleT>
            void _module(const ModuleT& overload){
                if(Visitables & visitable::callable){
                    auto info = overload.info();
                    
                    std::string method      = (boost::format("<div class='column udho-module-method'>%1%</div>")      % info._method).str();
                    std::string pattern     = (boost::format("<div class='column udho-module-pattern'>%1%</div>")     % info._pattern).str();
                    std::string fptr        = (boost::format("<div class='column udho-module-fptr'>%1%</div>")        % info._fptr).str();
                    std::string compositor  = (boost::format("<div class='column udho-module-compositor'>%1%</div>")  % info._compositor).str();
                    std::string module_html = (boost::format("<div class='udho-module'>%1% %2% %3% %4%</div>") % method % pattern % fptr % compositor).str();
                    
                    _stream << module_html << std::endl;
                }
            }
            template <typename AppT>
            void _application(const AppT& app){
                if(Visitables & visitable::application){                    
                    std::string name = (boost::format("<div class='column udho-application-name'>%1%</div>") % app.name()).str();
                    std::string path = (boost::format("<div class='column udho-application-path'>%1%</div>") % app._path).str();
                    std::string fptr = (boost::format("<div class='column udho-application-fptr'>%1%</div>") % &app).str();
                    std::string appb = (boost::format("<div class='udho-application'><div class='udho-application-heading'>%1% %2% %3%</div><div class='overloads udho-application-overloads'>") % name % path % fptr).str();
                    
                    _stream << appb << std::endl;
                }
            }
            void operator()(){
                std::string appe = "</div></div>";
                
                _stream << appe << std::endl;
            }
        };

        /**
         * prints the selected routes in the router
         * \code
         * router /= budho::visitors::print<budho::visitors::visitable::callable | budho::visitors::visitable::application, std::ostream>(std::cout);
         * \endcode
         */
        template <std::uint8_t Visitables, typename F>
        visitor<printing_visitor<Visitables, F>> print(F& callback){
            return visitor<printing_visitor<Visitables, F>>(callback);
        }
        
        template <std::uint8_t Visitables, typename F>
        visitor<printing_visitor_html<Visitables, F>> print_html(F& callback){
            return visitor<printing_visitor_html<Visitables, F>>(callback);
        }
        
        template <typename F, std::uint8_t Visitables, std::uint8_t HasModule = (Visitables & visitable::callable)>
        struct target_module{
            F _callback;
            target_module(F callback): _callback(callback){}
            
            template <typename ModuleT>
            void _module(ModuleT& overload){
                _callback(overload);
            }
        };
        template <typename F, std::uint8_t Visitables>
        struct target_module<F, Visitables, 0>{
            F _callback;
            target_module(F callback): _callback(callback){}
            
            template <typename ModuleT>
            void _module(ModuleT& /*overload*/){}
        };
        
        template <typename F, std::uint8_t Visitables, std::uint8_t HasApplication = (Visitables & visitable::application)>
        struct target_application{
            F _callback;
            target_application(F callback): _callback(callback){}
            
            template <typename AppT>
            void _application(AppT& overload){
                _callback(overload);
            }
        };
        template <typename F, std::uint8_t Visitables>
        struct target_application<F, Visitables, 0>{
            F _callback;
            target_application(F callback): _callback(callback){}
            
            template <typename AppT>
            void _application(AppT& /*overload*/){}
        };
        
        template <std::uint8_t Visitables, typename F>
        struct targetted_visitor: target_module<F, Visitables>, target_application<F, Visitables>{
            targetted_visitor(F callback): target_module<F, Visitables>(callback), target_application<F, Visitables>(callback){}        
            void operator()(){}
        };

        template <std::uint8_t Visitables, typename F>
        using targeted = visitor<targetted_visitor<Visitables, F>>;
        
        template <std::uint8_t Visitables, typename F>
        visitor<targetted_visitor<Visitables, F>> target(F callback){
            return visitor<targetted_visitor<Visitables, F>>(callback);
        }
    }
}

#endif // UDHO_VISITOR_H

